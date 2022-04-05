#include "GltfSceneLoader.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Asserts.h>
#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/Renderer/Scene.h>

#include <filesystem>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace PhxEngine::Core;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::ECS;
using namespace PhxEngine::RHI;
using namespace DirectX;

static bool TransformToLH = true;

class BufferRegionBlob : public IBlob
{
public:
	BufferRegionBlob(
		std::shared_ptr<IBlob> const& parent, size_t offset, size_t size)
		: m_parent(parent)
		, m_data(static_cast<const uint8_t*>(parent->Data()) + offset)
		, m_size(size)
	{
	}

	[[nodiscard]] const void* Data() const override
	{
		return this->m_data;
	}

	[[nodiscard]] const size_t Size() const override
	{
		return this->m_size;
	}

private:
	std::shared_ptr<IBlob> m_parent;
	const void* m_data;
	size_t m_size;
};

// glTF only support DDS images through the MSFT_texture_dds extension.
// Since cgltf does not support this extension, we parse the custom extension string as json here.
// See https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Vendor/MSFT_texture_dds 
static const cgltf_image* ParseDDSImage(const cgltf_texture* texture, const cgltf_data* objects)
{
	for (size_t i = 0; i < texture->extensions_count; i++)
	{
		const cgltf_extension& ext = texture->extensions[i];

		if (!ext.name || !ext.data)
		{
			continue;
		}

		if (strcmp(ext.name, "MSFT_texture_dds") != 0)
		{
			continue;
		}

		size_t extensionLength = strlen(ext.data);
		if (extensionLength > 1024)
		{
			return nullptr; // safeguard against weird inputs
		}

		jsmn_parser parser;
		jsmn_init(&parser);

		// count the tokens, normally there are 3
		int numTokens = jsmn_parse(&parser, ext.data, extensionLength, nullptr, 0);

		// allocate the tokens on the stack
		jsmntok_t* tokens = (jsmntok_t*)alloca(numTokens * sizeof(jsmntok_t));

		// reset the parser and prse
		jsmn_init(&parser);
		int numParsed = jsmn_parse(&parser, ext.data, extensionLength, tokens, numTokens);
		if (numParsed != numTokens)
		{
			LOG_CORE_WARN("Failed to parse the DDS glTF extension: %s", ext.data);
			return nullptr;
		}

		if (tokens[0].type != JSMN_OBJECT)
		{
			LOG_CORE_WARN("Failed to parse the DDS glTF extension: %s", ext.data);
			return nullptr;
		}

		for (int k = 1; k < numTokens; k++)
		{
			if (tokens[k].type != JSMN_STRING)
			{
				LOG_CORE_WARN("Failed to parse the DDS glTF extension: %s", ext.data);
				return nullptr;
			}

			if (cgltf_json_strcmp(tokens + k, (const uint8_t*)ext.data, "source") == 0)
			{
				++k;
				int index = cgltf_json_to_int(tokens + k, (const uint8_t*)ext.data);
				if (index < 0)
				{
					LOG_CORE_WARN("Failed to parse the DDS glTF extension: %s", ext.data);
					return nullptr;
				}

				if (size_t(index) >= objects->images_count)
				{
					LOG_CORE_WARN("Invalid image index %s specified in glTF texture definition", index);
					return nullptr;
				}

				return objects->images + index;
			}

			// this was something else - skip it
			k = cgltf_skip_json(tokens, k);
		}
	}

	return nullptr;
}

static std::pair<const uint8_t*, size_t> CgltfBufferAccessor(const cgltf_accessor* accessor, size_t defaultStride)
{
	// TODO: sparse accessor support
	const cgltf_buffer_view* view = accessor->buffer_view;
	const uint8_t* data = (uint8_t*)view->buffer->data + view->offset + accessor->offset;
	const size_t stride = view->stride ? view->stride : defaultStride;
	return std::make_pair(data, stride);
}

static void ComputeTangentSpace(
	const cgltf_accessor* cgltfPositionsAccessor,
	const cgltf_accessor* cgltfNormalsAccessor,
	const cgltf_accessor* cgltfTexCoordsAccessor,
	MeshComponent& mesh,
	const uint32_t indexCount,
	const uint32_t totalIndices,
	const uint32_t totalVertices)
{
	auto [positionSrc, positionStride] = CgltfBufferAccessor(cgltfPositionsAccessor, sizeof(float) * 3);
	auto [texcoordSrc, texcoordStride] = CgltfBufferAccessor(cgltfTexCoordsAccessor, sizeof(float) * 2);
	auto [normalSrc, normalStride] = CgltfBufferAccessor(cgltfNormalsAccessor, sizeof(float) * 3);

	std::vector<DirectX::XMVECTOR> computedTangents(cgltfPositionsAccessor->count);
	std::vector<DirectX::XMVECTOR> computedBitangents(cgltfPositionsAccessor->count);

	for (int i = 0; i < indexCount; i += 3)
	{
		auto& index0 = mesh.Indices[i + 0 + totalIndices];
		auto& index1 = mesh.Indices[i + 1 + totalIndices];
		auto& index2 = mesh.Indices[i + 2 + totalIndices];

		// Vertices
		DirectX::XMVECTOR pos0 = DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(positionSrc + positionStride * index0));
		DirectX::XMVECTOR pos1 = DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(positionSrc + positionStride * index1));
		DirectX::XMVECTOR pos2 = DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(positionSrc + positionStride * index2));

		// UVs
		DirectX::XMVECTOR uvs0 = DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(texcoordSrc + texcoordStride * index0));
		DirectX::XMVECTOR uvs1 = DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(texcoordSrc + texcoordStride * index1));
		DirectX::XMVECTOR uvs2 = DirectX::XMLoadFloat2(reinterpret_cast<const DirectX::XMFLOAT2*>(texcoordSrc + texcoordStride * index2));

		DirectX::XMVECTOR deltaPos1 = DirectX::XMVectorSubtract(pos1, pos0);
		DirectX::XMVECTOR deltaPos2 = DirectX::XMVectorSubtract(pos2, pos0);

		DirectX::XMVECTOR deltaUV1 = DirectX::XMVectorSubtract(uvs1, uvs0);
		DirectX::XMVECTOR deltaUV2 = DirectX::XMVectorSubtract(uvs2, uvs0);

		// TODO: Take advantage of SIMD better here
		float r = 1.0f / (DirectX::XMVectorGetX(deltaUV1) * DirectX::XMVectorGetY(deltaUV2) - DirectX::XMVectorGetY(deltaUV1) * DirectX::XMVectorGetX(deltaUV2));

		DirectX::XMVECTOR tangent = (deltaPos1 * DirectX::XMVectorGetY(deltaUV2) - deltaPos2 * DirectX::XMVectorGetY(deltaUV1)) * r;
		DirectX::XMVECTOR bitangent = (deltaPos2 * DirectX::XMVectorGetX(deltaUV1) - deltaPos1 * DirectX::XMVectorGetX(deltaUV2)) * r;

		computedTangents[index0] += tangent;
		computedTangents[index1] += tangent;
		computedTangents[index2] += tangent;

		computedBitangents[index0] += bitangent;
		computedBitangents[index1] += bitangent;
		computedBitangents[index2] += bitangent;
	}

	for (int i = 0; i < cgltfPositionsAccessor->count; i++)
	{
		const DirectX::XMVECTOR normal = DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(normalSrc + normalStride * i));
		const DirectX::XMVECTOR& tangent = computedTangents[i];
		const DirectX::XMVECTOR& bitangent = computedBitangents[i];

		// Gram-Schmidt orthogonalize
		DirectX::XMVECTOR orthTangent = DirectX::XMVector3Normalize(tangent - normal * DirectX::XMVector3Dot(normal, tangent));
		float sign = DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(normal, tangent), bitangent)) > 0
			? -1.0f
			: 1.0f;

		DirectX::XMVectorSetW(tangent, sign);
		DirectX::XMStoreFloat4(&mesh.VertexTangents[i + totalVertices], orthTangent);
	}
}

struct CgltfContext
{
	std::shared_ptr<IFileSystem> FileSystem;
	std::vector<std::shared_ptr<IBlob>> Blobs;
};

static cgltf_result CgltfReadFile(
	const struct cgltf_memory_options* memory_options,
	const struct cgltf_file_options* file_options,
	const char* path,
	cgltf_size* size,
	void** data)
{
	CgltfContext* context = (CgltfContext*)file_options->user_data;

	std::shared_ptr<IBlob> blob = context->FileSystem->ReadFile(path);

	if (!blob)
	{
		return cgltf_result_file_not_found;
	}

	context->Blobs.push_back(blob);

	if (size) *size = blob->Size();
	if (data) *data = (void*)blob->Data();  // NOLINT(clang-diagnostic-cast-qual)

	return cgltf_result_success;
}

void CgltfReleaseFile(
	const struct cgltf_memory_options*,
	const struct cgltf_file_options*, 
	void*)
{
	// do nothing
}

bool PhxEngine::Renderer::GltfSceneLoader::LoadScene(
	std::string const& fileName,
	RHI::CommandListHandle commandList,
	New::Scene& scene)
{
	this->m_filename = fileName; // Is this assignment safe?

	std::string normalizedFileName = this->m_filename.lexically_normal().generic_string();

	CgltfContext cgltfContext = {};
	cgltfContext.FileSystem = this->m_fileSystem;

	cgltf_options options = { };
	options.file.read = &CgltfReadFile;
	options.file.release = &CgltfReleaseFile;
	options.file.user_data = &cgltfContext;

	auto blob =  this->m_fileSystem->ReadFile(fileName);

	cgltf_data* objects = nullptr;
	cgltf_result res = cgltf_parse(&options, blob->Data(), blob->Size(), &objects);

	if (res != cgltf_result_success)
	{
		LOG_CORE_ERROR("Coouldn't load glTF file %s", normalizedFileName.c_str());
		return nullptr;
	}

	res = cgltf_load_buffers(&options, objects, normalizedFileName.c_str());
	if (res != cgltf_result_success)
	{
		LOG_CORE_ERROR("Failed to load buffers for glTF file '%s'", normalizedFileName.c_str());
		return false;
	}

	return this->LoadSceneInternal(objects, cgltfContext, commandList, scene);
}

bool PhxEngine::Renderer::GltfSceneLoader::LoadSceneInternal(
	cgltf_data* gltfData,
	CgltfContext& context,
	RHI::CommandListHandle commandList,
	New::Scene& scene)
{
	this->LoadMaterialData(
		gltfData->materials,
		gltfData->materials_count,
		gltfData,
		context,
		commandList,
		scene);

	this->LoadMeshData(
		gltfData->meshes,
		gltfData->meshes_count,
		scene);

	Entity rootEntity = CreateEntity();
	scene.Transforms.Create(rootEntity);
	scene.Names.Create(rootEntity) = gltfData->scene->name ? gltfData->scene->name : "GLTF Scene"; // TODO: Use filename.
	
	
	// Load Node Data
	for (size_t i = 0; i < gltfData->scene->nodes_count; i++)
	{
		// Load Node Data
		this->LoadNode(*gltfData->scene->nodes[i], rootEntity, scene);
	}

	return true;
}

PhxEngine::RHI::TextureHandle PhxEngine::Renderer::GltfSceneLoader::LoadTexture(
	const cgltf_texture* cglftTexture,
	bool isSRGB,
	const cgltf_data* objects,
	CgltfContext& context,
	RHI::CommandListHandle commandList)
{
	if (!cglftTexture)
	{
		return nullptr;
	}

	const cgltf_image* ddsImage = ParseDDSImage(cglftTexture, objects);

	if ((!cglftTexture->image || (!cglftTexture->image->uri && !cglftTexture->image->buffer_view)) && (!ddsImage || (!ddsImage->uri && !ddsImage->buffer_view)))
	{
		return nullptr;
	}

	// Pick either DDS or standard image, prefer DDS
	const cgltf_image* activeImage = (ddsImage && (ddsImage->uri || ddsImage->buffer_view)) ? ddsImage : cglftTexture->image;

	const uint64_t imageIndex = activeImage - objects->images;
	std::string name = activeImage->name ? activeImage->name : this->m_filename.filename().generic_string() + "[" + std::to_string(imageIndex) + "]";
	std::string mimeType = activeImage->mime_type ? activeImage->mime_type : "";

	auto texture = this->m_textureCache->GetTexture(name);
	if (texture)
	{
		return texture;
	}

	if (activeImage->buffer_view)
	{
		const uint8_t* dataPtr = static_cast<uint8_t*>(activeImage->buffer_view->data) + activeImage->buffer_view->offset;
		const size_t dataSize = activeImage->buffer_view->size;

		std::shared_ptr<IBlob> textureData;

		for (const auto& blob : context.Blobs)
		{
			const uint8_t* blobData = static_cast<const uint8_t*>(blob->Data());
			const size_t blobSize = blob->Size();

			if (blobData < dataPtr && blobData + blobSize > dataPtr)
			{
				// Found the file blob - create a range blob out of it and keep a strong reference.
				assert(dataPtr + dataSize <= blobData + blobSize);
				textureData = std::make_shared<BufferRegionBlob>(blob, dataPtr - blobData, dataSize);
				break;
			}
		}

		if (!textureData)
		{
			void* dataCopy = malloc(dataSize);
			assert(dataCopy);
			memcpy(dataCopy, dataPtr, dataSize);
			textureData = std::make_shared<Core::Blob>(dataCopy, dataSize);
		}
		
		texture = this->m_textureCache->LoadTexture(textureData, name, mimeType, isSRGB, commandList);
	}
	else
	{
		texture = this->m_textureCache->LoadTexture(this->m_filename.parent_path() / activeImage->uri, isSRGB, commandList);
	}

	return texture;
}

void PhxEngine::Renderer::GltfSceneLoader::LoadMaterialData(
	const cgltf_material* pMaterials,
	uint32_t materialCount,
	const cgltf_data* objects,
	CgltfContext& context,
	RHI::CommandListHandle commandList,
	New::Scene& scene)
{
	for (int i = 0; i < materialCount; i++)
	{
		const auto& cgltfMtl = pMaterials[i];

		std::string mtlName = cgltfMtl.name ? cgltfMtl.name : "Material " + std::to_string(i);

		Entity mtlEntity = scene.EntityCreateMaterial(mtlName);
		this->m_materialMap[&cgltfMtl] = mtlEntity;
		MaterialComponent& mtl = *scene.Materials.GetComponent(mtlEntity);

		if (cgltfMtl.has_pbr_specular_glossiness)
		{
			LOG_CORE_WARN("Material %s contains unsupported extension 'PBR Specular_Glossiness' workflow ");
		}
		else if (cgltfMtl.has_pbr_metallic_roughness)
		{
			mtl.AlbedoTexture = this->LoadTexture(
				cgltfMtl.pbr_metallic_roughness.base_color_texture.texture,
				true,
				objects,
				context,
				commandList);

			mtl.Albedo =
			{
				cgltfMtl.pbr_metallic_roughness.base_color_factor[0],
				cgltfMtl.pbr_metallic_roughness.base_color_factor[1],
				cgltfMtl.pbr_metallic_roughness.base_color_factor[2],
				cgltfMtl.pbr_metallic_roughness.base_color_factor[3]
			};

			mtl.MetalRoughnessTexture = this->LoadTexture(
				cgltfMtl.pbr_metallic_roughness.metallic_roughness_texture.texture,
				false,
				objects,
				context,
				commandList);

			mtl.Metalness = cgltfMtl.pbr_metallic_roughness.metallic_factor;
			mtl.Roughness = cgltfMtl.pbr_metallic_roughness.roughness_factor;
		}

		// Load Normal map
		mtl.NormalMapTexture = this->LoadTexture(
			cgltfMtl.normal_texture.texture,
			false,
			objects,
			context,
			commandList);

		// TODO: Emmisive
		// TODO: Aplha suppor
		mtl.IsDoubleSided = cgltfMtl.double_sided;
	}

	if (scene.Materials.IsEmpty())
	{
		scene.EntityCreateMaterial("Default_Material");
	}
}

void PhxEngine::Renderer::GltfSceneLoader::LoadMeshData(
	const cgltf_mesh* pMeshes,
	uint32_t meshCount,
	New::Scene& scene)
{
	std::vector<size_t> totalVertexCounts(meshCount);
	std::vector<size_t> totalIndexCounts(meshCount);

	for (int i = 0; i < meshCount; i++)
	{
		const auto& cgltfMesh = pMeshes[i];

		for (int iPrim = 0; iPrim < cgltfMesh.primitives_count; iPrim++)
		{
			const auto& cgltfPrim = cgltfMesh.primitives[iPrim];

			if (cgltfPrim.type != cgltf_primitive_type_triangles ||
				cgltfPrim.attributes_count == 0)
			{
				continue;
			}

			if (cgltfPrim.indices)
			{
				totalIndexCounts[i] += cgltfPrim.indices->count;
			}
			else
			{
				totalIndexCounts[i] += cgltfPrim.attributes->data->count;
			}

			totalVertexCounts[i] += cgltfPrim.attributes->data->count;
		}
	}
	/*
	auto meshBuffers = std::make_shared<MeshBuffers>();
	meshBuffers->IndexData.resize(totalIndexCounts);
	meshBuffers->PositionData.resize(totalVertexCounts);
	meshBuffers->NormalData.resize(totalVertexCount);
	meshBuffers->TexCoordData.resize(totalVertexCount);
	meshBuffers->TangentData.resize(totalVertexCount);
	*/
	for (int i = 0; i < meshCount; i++)
	{
		const auto& cgltfMesh = pMeshes[i];
		std::string meshName = cgltfMesh.name ? cgltfMesh.name : "Mesh " + std::to_string(i);

		Entity meshEntity = scene.EntityCreateMesh(meshName);
		this->m_meshMap[&cgltfMesh] = meshEntity;
		MeshComponent& mesh = *scene.Meshes.GetComponent(meshEntity);

		// Resize data
		mesh.VertexPositions.resize(totalVertexCounts[i]);
		mesh.VertexNormals.resize(totalVertexCounts[i]);
		mesh.VertexTexCoords.resize(totalVertexCounts[i]);
		mesh.VertexTangents.resize(totalVertexCounts[i]);
		mesh.Indices.resize(totalIndexCounts[i]);

		mesh.IndexOffset = 0;
		mesh.VertexOffset = 0;

		mesh.Geometry.resize(cgltfMesh.primitives_count);
		size_t totalIndexCount = 0;
		size_t totalVertexCount = 0;

		for (int iPrim = 0; iPrim < cgltfMesh.primitives_count; iPrim++)
		{
			const auto& cgltfPrim = cgltfMesh.primitives[iPrim];

			if (cgltfPrim.type != cgltf_primitive_type_triangles ||
				cgltfPrim.attributes_count == 0)
			{
				continue;
			}

			if (cgltfPrim.indices)
			{
				PHX_ASSERT(cgltfPrim.indices->component_type == cgltf_component_type_r_32u ||
					cgltfPrim.indices->component_type == cgltf_component_type_r_16u ||
					cgltfPrim.indices->component_type == cgltf_component_type_r_8u);
				PHX_ASSERT(cgltfPrim.indices->type == cgltf_type_scalar);
			}

			const cgltf_accessor* cgltfPositionsAccessor = nullptr;
			const cgltf_accessor* cgltfTangentsAccessor = nullptr;
			const cgltf_accessor* cgltfNormalsAccessor = nullptr;
			const cgltf_accessor* cgltfTexCoordsAccessor = nullptr;

			// Collect intreasted attributes
			for (int iAttr = 0; iAttr < cgltfPrim.attributes_count; iAttr++)
			{
				const auto& cgltfAttribute = cgltfPrim.attributes[iAttr];

				switch (cgltfAttribute.type)
				{
				case cgltf_attribute_type_position:
					PHX_ASSERT(cgltfAttribute.data->type == cgltf_type_vec3);
					PHX_ASSERT(cgltfAttribute.data->component_type== cgltf_component_type_r_32f);
					cgltfPositionsAccessor = cgltfAttribute.data;
					break;

				case cgltf_attribute_type_tangent:
					PHX_ASSERT(cgltfAttribute.data->type == cgltf_type_vec4);
					PHX_ASSERT(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
					cgltfTangentsAccessor = cgltfAttribute.data;
					break;

				case cgltf_attribute_type_normal:
					PHX_ASSERT(cgltfAttribute.data->type == cgltf_type_vec3);
					PHX_ASSERT(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
					cgltfNormalsAccessor = cgltfAttribute.data;
					break;

				case cgltf_attribute_type_texcoord:
					PHX_ASSERT(cgltfAttribute.data->type == cgltf_type_vec2);
					PHX_ASSERT(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
					cgltfTexCoordsAccessor = cgltfAttribute.data;
					break;
				}
			}

			PHX_ASSERT(cgltfPositionsAccessor);

			uint32_t indexCount = 0;

			if (cgltfPrim.indices)
			{
				indexCount = cgltfPrim.indices->count;

				// copy the indices
				auto [indexSrc, indexStride] = CgltfBufferAccessor(cgltfPrim.indices, 0);

				uint32_t* indexDst = mesh.Indices.data() + totalIndexCount;
				switch (cgltfPrim.indices->component_type)
				{
				case cgltf_component_type_r_8u:
					if (!indexStride)
					{
						indexStride = sizeof(uint8_t);
					}

					for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
					{
						*indexDst = *(const uint8_t*)indexSrc + totalVertexCount;

						indexSrc += indexStride;
						indexDst++;
					}
					break;

				case cgltf_component_type_r_16u:
					if (!indexStride)
					{
						indexStride = sizeof(uint16_t);
					}

					for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
					{
						*indexDst = *(const uint16_t*)indexSrc + totalVertexCount;;

						indexSrc += indexStride;
						indexDst++;
					}
					break;

				case cgltf_component_type_r_32u:
					if (!indexStride)
					{
						indexStride = sizeof(uint32_t);
					}

					for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
					{
						*indexDst = *(const uint32_t*)indexSrc + totalVertexCount;;

						indexSrc += indexStride;
						indexDst++;
					}
					break;

				default:
					PHX_ASSERT(false);
				}
			}
			else
			{
				indexCount = cgltfPositionsAccessor->count;

				// generate the indices
				uint32_t* indexDst = mesh.Indices.data() + totalIndexCount;
				for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
				{
					*indexDst = (uint32_t)iIdx;
					indexDst++;
				}
			}

			// Process Position data
			if (cgltfPositionsAccessor)
			{
				auto [positionSrc, positionStride] = CgltfBufferAccessor(cgltfPositionsAccessor, sizeof(float) * 3);

				// Do a mem copy?
				std::memcpy(
					mesh.VertexPositions.data() + totalVertexCount,
					positionSrc,
					positionStride* cgltfPositionsAccessor->count);
			}

			if (cgltfNormalsAccessor)
			{
				auto [normalSrc, normalStride] = CgltfBufferAccessor(cgltfNormalsAccessor, sizeof(float) * 3);

				// Do a mem copy?
				std::memcpy(
					mesh.VertexNormals.data() + totalVertexCount,
					normalSrc,
					normalStride * cgltfNormalsAccessor->count);
			}

			if (cgltfTangentsAccessor)
			{
				auto [tangentSrc, tangentStride] = CgltfBufferAccessor(cgltfTangentsAccessor, sizeof(float) * 4);

				// Do a mem copy?
				std::memcpy(
					mesh.VertexTangents.data() + totalVertexCount,
					tangentSrc,
					tangentStride* cgltfTangentsAccessor->count);
			}

			if (cgltfTexCoordsAccessor)
			{
				PHX_ASSERT(cgltfTexCoordsAccessor->count == cgltfPositionsAccessor->count);

				auto [texcoordSrc, texcoordStride] = CgltfBufferAccessor(cgltfTexCoordsAccessor, sizeof(float) * 2);

				std::memcpy(
					mesh.VertexTexCoords.data() + totalVertexCount,
					texcoordSrc,
					texcoordStride* cgltfTexCoordsAccessor->count);
			}
			else
			{
				std::memset(
					mesh.VertexTexCoords.data() + totalVertexCount,
					0.0f,
					cgltfPositionsAccessor->count * sizeof(float) * 2);
			}

			if (cgltfNormalsAccessor && cgltfTangentsAccessor && !cgltfTangentsAccessor)
			{
				ComputeTangentSpace(
					cgltfPositionsAccessor,
					cgltfNormalsAccessor,
					cgltfTexCoordsAccessor,
					mesh,
					indexCount,
					totalIndexCount,
					totalVertexCount);
			}

			auto& meshGeometry = mesh.Geometry[iPrim];
			{
				auto it = this->m_materialMap.find(cgltfPrim.material);
				meshGeometry.MaterialID = it == this->m_materialMap.end() ? InvalidEntity : it->second;
			}

			meshGeometry.IndexOffsetInMesh = mesh.TotalIndices;
			meshGeometry.VertexOffsetInMesh = mesh.TotalVertices;
			meshGeometry.NumIndices = indexCount;
			meshGeometry.NumVertices = cgltfPositionsAccessor->count;

			mesh.TotalIndices += meshGeometry.NumIndices;
			mesh.TotalVertices += meshGeometry.NumVertices;

			totalIndexCount += meshGeometry.NumIndices;
			totalVertexCount += meshGeometry.NumVertices;
		}
	}
}

void PhxEngine::Renderer::GltfSceneLoader::LoadNode(
	const cgltf_node& gltfNode,
	Entity parent,
	New::Scene& scene)
{

	Entity entity = InvalidEntity;
	if (gltfNode.mesh)
	{
		// Create a mesh instance
		static size_t meshId = 0;

		std::string nodeName = gltfNode.name ? gltfNode.name : "Scene Node " + std::to_string(meshId++);
		entity = scene.EntityCreateMeshInstance(nodeName);
		auto& instanceComp = *scene.MeshInstances.GetComponent(entity);
		instanceComp.MeshId = this->m_meshMap[gltfNode.mesh];
	}
	else if (gltfNode.camera)
	{
		static size_t cameraId = 0;
		std::string cameraName = gltfNode.name ? gltfNode.name : "Camera " + std::to_string(cameraId++);
		entity = scene.EntityCreateCamera(
			cameraName,
			scene.GetGlobalCamera().Width,
			scene.GetGlobalCamera().Height,
			0.1f,
			1000.0f);
	}

	if (entity == InvalidEntity)
	{
		entity = CreateEntity();
		scene.Transforms.Create(entity);

		static size_t nodeId = 0;
		std::string nodeName = gltfNode.name ? gltfNode.name : "Scene Node " + std::to_string(nodeId++);
		scene.Names.Create(entity).Name = nodeName;
	}

	// Create an entity map that can be used?
	TransformComponent& transform = *scene.Transforms.GetComponent(entity);
	if (gltfNode.has_scale)
	{
		std::memcpy(
			&transform.LocalScale.x,
			&gltfNode.scale[0],
			sizeof(float) * 3);
	}
	if (gltfNode.has_rotation)
	{
		std::memcpy(
			&transform.LocalRotation.x,
			&gltfNode.rotation[0],
			sizeof(float) * 4);
	}
	if (gltfNode.has_translation)
	{
		std::memcpy(
			&transform.LocalTranslation.x,
			&gltfNode.translation[0],
			sizeof(float) * 3);
	}
	if (gltfNode.has_matrix)
	{
		std::memcpy(
			&transform.WorldMatrix._11,
			&gltfNode.matrix[0],
			sizeof(float) * 16);

		transform.ApplyTransform();
	}

	transform.UpdateTransform();

	if (parent != InvalidEntity)
	{
		scene.ComponentAttach(entity, parent, true);
	}

	for (int i = 0; i < gltfNode.children_count; i++)
	{
		if (gltfNode.children[i])
			this->LoadNode(*gltfNode.children[i], entity, scene);
	}
}
