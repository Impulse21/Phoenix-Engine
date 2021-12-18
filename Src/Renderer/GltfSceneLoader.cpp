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
using namespace PhxEngine::RHI;
using namespace DirectX;

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
			LOG_CORE_WARN("Failed to parse the DDS glTF extension: {0}", ext.data);
			return nullptr;
		}

		if (tokens[0].type != JSMN_OBJECT)
		{
			LOG_CORE_WARN("Failed to parse the DDS glTF extension: {0}", ext.data);
			return nullptr;
		}

		for (int k = 1; k < numTokens; k++)
		{
			if (tokens[k].type != JSMN_STRING)
			{
				LOG_CORE_WARN("Failed to parse the DDS glTF extension: {0}", ext.data);
				return nullptr;
			}

			if (cgltf_json_strcmp(tokens + k, (const uint8_t*)ext.data, "source") == 0)
			{
				++k;
				int index = cgltf_json_to_int(tokens + k, (const uint8_t*)ext.data);
				if (index < 0)
				{
					LOG_CORE_WARN("Failed to parse the DDS glTF extension: {0}", ext.data);
					return nullptr;
				}

				if (size_t(index) >= objects->images_count)
				{
					LOG_CORE_WARN("Invalid image index {0} specified in glTF texture definition", index);
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
	std::shared_ptr<MeshBuffers> meshBuffers,
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
		auto& index0 = meshBuffers->IndexData[i + 0 + totalIndices];
		auto& index1 = meshBuffers->IndexData[i + 1 + totalIndices];
		auto& index2 = meshBuffers->IndexData[i + 2 + totalIndices];

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
		DirectX::XMStoreFloat4(&meshBuffers->TangentData[i + totalVertices], orthTangent);
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

	auto blob = context->FileSystem->ReadFile(path);

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

Scene* PhxEngine::Renderer::GltfSceneLoader::LoadScene(std::string const& filename)
{
	this->m_filename = filename; // Is this assignment safe?

	std::string normalizedFileName = this->m_filename.lexically_normal().generic_string();

	CgltfContext cgltfContext = {};
	cgltfContext.FileSystem = this->m_fileSystem;

	cgltf_options options = { };
	options.file.read = &CgltfReadFile;
	options.file.release = &CgltfReleaseFile;
	options.file.user_data = &cgltfContext;

	cgltf_data* objects = nullptr;
	cgltf_result res = cgltf_parse_file(&options, normalizedFileName.c_str(), &objects);

	if (res != cgltf_result_success)
	{
		LOG_CORE_ERROR("Coouldn't load glTF file {0}", normalizedFileName.c_str());
		return nullptr;
	}


	res = cgltf_load_buffers(&options, objects, normalizedFileName.c_str());
	if (res != cgltf_result_success)
	{
		LOG_CORE_ERROR("Failed to load buffers for glTF file '{0}'", normalizedFileName.c_str());
		return false;
	}
	this->LoadSceneInternal(objects, cgltfContext);

    return nullptr;
}

Scene* PhxEngine::Renderer::GltfSceneLoader::LoadSceneInternal(
	cgltf_data* gltfData,
	CgltfContext& context)
{
	/*
	Scene scene;

	// load textures
	std::unordered_map<const cgltf_material*, std::shared_ptr<Material>> sceneMaterials;
	this->LoadMaterialData(
		gltfData->materials,
		gltfData->materials_count,
		gltfData,
		context,
		sceneMaterials);

	std::unordered_map<const cgltf_mesh*, std::shared_ptr<cgltf_mesh>> sceneMeshes;
	this->LoadMeshData(
		gltfData->meshes,
		gltfData->meshes_count,
		sceneMaterials,
		sceneMeshes);

	// TODO: Camera

	// TODO: Lights

	// TODO: Build Scene Graph
	*/
	return nullptr;
}

std::shared_ptr<Texture> PhxEngine::Renderer::GltfSceneLoader::LoadTexture(
	const cgltf_texture* cglftTexture,
	bool isSRGB,
	const cgltf_data* objects,
	CgltfContext& context)
{
	if (cglftTexture)
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
			textureData = std::make_shared<Blob>(dataCopy, dataSize);
		}

		// texture = this->m_textureCache->LoadTexture(textureData, name, mimeType, isSRGB);
	}
	else
	{
		// texture = this->m_textureCache->LoadTexture(this->m_filename.parent_path() / activeImage->uri, isSRGB);
	}

	// Load texture

	return std::shared_ptr<Texture>();
}

void PhxEngine::Renderer::GltfSceneLoader::LoadMaterialData(
	const cgltf_material* pMaterials,
	uint32_t materialCount,
	const cgltf_data* objects,
	CgltfContext& context,
	std::unordered_map<const cgltf_material*, std::shared_ptr<Material>>& outMaterials)
{
	for (int i = 0; i < materialCount; i++)
	{
		const auto& cgltfMaterial = pMaterials[i];

		auto material = std::make_shared<Material>();

		if (cgltfMaterial.name)
		{
			material->Name = cgltfMaterial.name;
		}

		if (cgltfMaterial.has_pbr_specular_glossiness)
		{
			LOG_CORE_WARN("Material {0} contains unsupported extension 'PBR Specular_Glossiness' workflow ");
		}
		else if (cgltfMaterial.has_pbr_metallic_roughness)
		{
			material->AlbedoTexture = this->LoadTexture(
				cgltfMaterial.pbr_metallic_roughness.base_color_texture.texture,
				true,
				objects,
				context);

			cgltfMaterial.pbr_metallic_roughness.metallic_roughness_texture;
			material->Albedo =
			{
				cgltfMaterial.pbr_metallic_roughness.base_color_factor[0],
				cgltfMaterial.pbr_metallic_roughness.base_color_factor[1],
				cgltfMaterial.pbr_metallic_roughness.base_color_factor[2],
			};

			material->MaterialTexture = this->LoadTexture(
				cgltfMaterial.pbr_metallic_roughness.metallic_roughness_texture.texture,
				false,
				objects,
				context);

			material->Metalness = cgltfMaterial.pbr_metallic_roughness.metallic_factor;
			material->Roughness = cgltfMaterial.pbr_metallic_roughness.roughness_factor;
		}

		// TODO: Emmisive
		// TODO: Aplha support
		material->IsDoubleSided = cgltfMaterial.double_sided;

		outMaterials[&cgltfMaterial] = material;
	}
}

void PhxEngine::Renderer::GltfSceneLoader::LoadMeshData(
	const cgltf_mesh* pMeshes,
	uint32_t meshCount,
	std::unordered_map<const cgltf_material*, std::shared_ptr<Material>> const& materialMap,
	std::unordered_map<const cgltf_mesh*, std::shared_ptr<cgltf_mesh>>& outMeshes)
{
	size_t totalVertexCount = 0;
	size_t totalIndexCount = 0;

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
				totalIndexCount += cgltfPrim.indices->count;
			}
			else
			{
				totalIndexCount += cgltfPrim.attributes->data->count;
			}

			totalVertexCount += cgltfPrim.attributes->data->count;
		}
	}

	auto meshBuffers = std::make_shared<MeshBuffers>();
	meshBuffers->IndexData.resize(totalIndexCount);
	meshBuffers->PositionData.resize(totalVertexCount);
	meshBuffers->NormalData.resize(totalVertexCount);
	meshBuffers->TexCoordData.resize(totalVertexCount);
	meshBuffers->TangentData.resize(totalVertexCount);


	totalIndexCount = 0;
	totalVertexCount = 0;

	std::vector<std::shared_ptr<Mesh>> meshes(meshCount);

	for (int i = 0; i < meshCount; i++)
	{
		const auto& cgltfMesh = pMeshes[i];
		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

		if (cgltfMesh.name)
		{
			mesh->Name = cgltfMesh.name;
		}

		mesh->Buffers = meshBuffers;
		mesh->IndexOffset = totalIndexCount;
		mesh->VertexOffset = totalVertexCount;

		meshes[i] = mesh;
		// outMeshes[&cgltfMesh] = mesh;

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

				uint32_t* indexDst = meshBuffers->IndexData.data() + totalIndexCount;

				switch (cgltfPrim.indices->component_type)
				{
				case cgltf_component_type_r_8u:
					if (!indexStride)
					{
						indexStride = sizeof(uint8_t);
					}

					for (size_t iIdx = 0; iIdx < indexCount; iIdx++)
					{
						*indexDst = *(const uint8_t*)indexSrc;

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
						*indexDst = *(const uint16_t*)indexSrc;

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
						*indexDst = *(const uint32_t*)indexSrc;

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
				uint32_t* indexDst = meshBuffers->IndexData.data() + totalIndexCount;
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
					meshBuffers->PositionData.data() + totalVertexCount,
					positionSrc,
					positionStride* cgltfPositionsAccessor->count);
			}

			if (cgltfNormalsAccessor)
			{
				auto [normalSrc, normalStride] = CgltfBufferAccessor(cgltfNormalsAccessor, sizeof(float) * 3);

				// Do a mem copy?
				std::memcpy(
					meshBuffers->NormalData.data() + totalVertexCount,
					normalSrc,
					normalStride * cgltfNormalsAccessor->count);
			}

			if (cgltfTangentsAccessor)
			{
				auto [tangentSrc, tangentStride] = CgltfBufferAccessor(cgltfTangentsAccessor, sizeof(float) * 4);

				// Do a mem copy?
				std::memcpy(
					meshBuffers->TangentData.data() + totalVertexCount,
					tangentSrc,
					tangentStride* cgltfTangentsAccessor->count);
			}

			if (cgltfTexCoordsAccessor)
			{
				PHX_ASSERT(cgltfTexCoordsAccessor->count == cgltfPositionsAccessor->count);

				auto [texcoordSrc, texcoordStride] = CgltfBufferAccessor(cgltfTexCoordsAccessor, sizeof(float) * 2);

				std::memcpy(
					meshBuffers->TexCoordData.data() + totalVertexCount,
					texcoordSrc,
					texcoordStride* cgltfTexCoordsAccessor->count);
			}
			else
			{
				DirectX::XMFLOAT2* texcoordDst = meshBuffers->TexCoordData.data() + totalVertexCount;
				std::memset(
					meshBuffers->TexCoordData.data() + totalVertexCount,
					0.0f,
					cgltfPositionsAccessor->count * sizeof(float) * 2);
			}

			if (cgltfNormalsAccessor && cgltfTangentsAccessor && !cgltfTangentsAccessor)
			{
				ComputeTangentSpace(
					cgltfPositionsAccessor,
					cgltfNormalsAccessor,
					cgltfTexCoordsAccessor,
					meshBuffers,
					indexCount,
					totalIndexCount,
					totalVertexCount);
			}

			// Construct Geometry
			auto meshGeometry = std::make_shared<MeshGeometry>();
			meshGeometry->Material = materialMap.at(cgltfPrim.material);
			meshGeometry->IndexOffsetInMesh = mesh->TotalIndices;
			meshGeometry->VertexOffsetInMesh = mesh->TotalVertices;
			meshGeometry->NumIndices = indexCount;
			meshGeometry->NumVertices = cgltfPositionsAccessor->count;

			mesh->TotalIndices += meshGeometry->NumIndices;
			mesh->VertexOffset += meshGeometry->NumVertices;
			mesh->Geometry.push_back(meshGeometry);

			totalIndexCount += meshGeometry->NumIndices;
			totalVertexCount += meshGeometry->NumVertices;
		}
	}
}
