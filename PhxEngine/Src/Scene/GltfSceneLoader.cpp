#include "phxpch.h"

#include <memory>
#include "GltfSceneLoader.h"
#include <PhxEngine/Graphics/TextureCache.h>

#include "PhxEngine/Scene/Scene.h"
#include "PhxEngine/Core/Helpers.h"
#include <PhxEngine/Core/VirtualFileSystem.h>
#include "PhxEngine/Scene/Components.h"
#include <PhxEngine/Shaders/ShaderInterop.h>

#include <filesystem>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;
using namespace PhxEngine::RHI;
using namespace DirectX;


namespace
{
	constexpr bool cReverseWinding = true;
	constexpr bool cReverseZ = true;
	constexpr const size_t cIndexRemap[] = { 0,2,1 };

}

// TODO: Use Span
/*
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
*/

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
			// // LOG_CORE_WARN("Failed to parse the DDS glTF extension: %s", ext.data);
			return nullptr;
		}

		if (tokens[0].type != JSMN_OBJECT)
		{
			// // LOG_CORE_WARN("Failed to parse the DDS glTF extension: %s", ext.data);
			return nullptr;
		}

		for (int k = 1; k < numTokens; k++)
		{
			if (tokens[k].type != JSMN_STRING)
			{
				// // LOG_CORE_WARN("Failed to parse the DDS glTF extension: %s", ext.data);
				return nullptr;
			}

			if (cgltf_json_strcmp(tokens + k, (const uint8_t*)ext.data, "source") == 0)
			{
				++k;
				int index = cgltf_json_to_int(tokens + k, (const uint8_t*)ext.data);
				if (index < 0)
				{
					// // LOG_CORE_WARN("Failed to parse the DDS glTF extension: %s", ext.data);
					return nullptr;
				}

				if (size_t(index) >= objects->images_count)
				{
					// // LOG_CORE_WARN("Invalid image index %s specified in glTF texture definition", index);
					return nullptr;
				}

				cgltf_image* retVal = objects->images + index;

				// Code To catch mismatch in DDS textures
				// assert(std::filesystem::path(retVal->uri).replace_extension(".png").filename().string() == std::filesystem::path(texture->image->uri).filename().string());

				return retVal;
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
	const uint8_t* Data = (uint8_t*)view->buffer->data + view->offset + accessor->offset;
	const size_t stride = view->stride ? view->stride : defaultStride;
	return std::make_pair(Data, stride);
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
	void** Data)
{
	CgltfContext* context = (CgltfContext*)file_options->user_data;

	std::unique_ptr<IBlob> dataBlob = context->FileSystem->ReadFile(path);
	if (!dataBlob)
	{
		return cgltf_result_file_not_found;
	}

	if (size)
	{
		*size = dataBlob->Size();
	}
		
	if (Data)
	{
		*Data = (void*)dataBlob->Data();  // NOLINT(clang-diagnostic-cast-qual)
	}

	context->Blobs.push_back(std::move(dataBlob));

	return cgltf_result_success;
}

void CgltfReleaseFile(
	const struct cgltf_memory_options*,
	const struct cgltf_file_options*, 
	void*)
{
	// do nothing
}
// --------------------------------------------------------------------------------


// TODO: Fix me
GltfSceneLoader::GltfSceneLoader()
{

}
bool GltfSceneLoader::LoadScene(
	std::shared_ptr<Core::IFileSystem> fileSystem,
	std::shared_ptr<Graphics::TextureCache> textureCache,
	std::filesystem::path const& fileName,
	RHI::ICommandList* commandList,
	PhxEngine::Scene::Scene& scene)
{
	this->m_textureCache = textureCache;

	this->m_filename = fileName; // Is this assignment safe?

	std::string normalizedFileName = this->m_filename.lexically_normal().generic_string();

	CgltfContext cgltfContext =
	{
		.FileSystem = fileSystem,
		.Blobs = {}
	};

	cgltf_options options = { };
	options.file.read = &CgltfReadFile;
	options.file.release = &CgltfReleaseFile;
	options.file.user_data = &cgltfContext;

	std::unique_ptr<IBlob> blob = fileSystem->ReadFile(fileName);
	if (!blob)
	{
		return false;
	}

	cgltf_data* objects = nullptr;
	cgltf_result res = cgltf_parse(&options, blob->Data(), blob->Size(), &objects);

	if (res != cgltf_result_success)
	{
		// LOG_CORE_ERROR("Coouldn't load glTF file %s", normalizedFileName.c_str());
		return false;
	}

	res = cgltf_load_buffers(&options, objects, normalizedFileName.c_str());
	if (res != cgltf_result_success)
	{
		// LOG_CORE_ERROR("Failed to load buffers for glTF file '%s'", normalizedFileName.c_str());
		return false;
	}

	return this->LoadSceneInternal(objects, cgltfContext, commandList, scene);
}

bool GltfSceneLoader::LoadSceneInternal(
	cgltf_data* gltfData,
	CgltfContext& context,
	ICommandList* commandList,
	Scene& scene)
{
	// Create a top level node
	std::string rootName = this->m_filename.stem().generic_string();;
	this->m_rootNode = scene.CreateEntity(rootName);

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
		commandList,
		scene);

#ifdef CREATE_DEFAULT_CAMERA
	// Add a default Camera for testing
	std::string cameraName = "Default Camera";

	ECS::ECS::Entity entity = scene.EntityCreateCamera(cameraName);
	auto& cameraComponent = *scene.Cameras.GetComponent(entity);
	cameraComponent.Width = Scene::GetGlobalCamera().Width;
	cameraComponent.Height = Scene::GetGlobalCamera().Height;
	cameraComponent.FoV = 1.7;
	TransformComponent& transform = *scene.Transforms.GetComponent(entity);
	transform.UpdateTransform();
	scene.ComponentAttach(entity, scene.RootEntity, true);
#endif

	// Load Node Data
	for (size_t i = 0; i < gltfData->scene->nodes_count; i++)
	{
		// Load Node Data
		this->LoadNode(*gltfData->scene->nodes[i], this->m_rootNode, scene);
	}
	
	scene.UpdateBounds();
	scene.SetBrdfLut(this->m_textureCache->LoadTexture("Assets\\Textures\\IBL\\BrdfLut.dds", true, commandList));
	return true;
}

void GltfSceneLoader::LoadNode(
	const cgltf_node& gltfNode,
	PhxEngine::Scene::Entity parent,
	Scene& scene)
{
	PhxEngine::Scene::Entity entity;

	// TODO: Use temp allocator
	std::vector<Entity> childEntities;
	if (gltfNode.mesh)
	{
		// Create a mesh instance
		static size_t meshId = 0;

		std::string nodeName = gltfNode.name ? gltfNode.name : "Scene Node " + std::to_string(meshId++);
		entity = scene.CreateEntity(nodeName);

		childEntities.reserve(this->m_meshEntityMap[gltfNode.mesh].size());
		uint32_t meshInstance = 0;
		for (auto& mesh : this->m_meshEntityMap[gltfNode.mesh])
		{
			std::string meshNodeName = nodeName + std::to_string(meshInstance++);
			PhxEngine::Scene::Entity subEntity = scene.CreateEntity(meshNodeName);

			auto& instanceComponent = subEntity.AddComponent<MeshInstanceComponent>();
			instanceComponent.Mesh = mesh;

			subEntity.AddComponent<AABBComponent>();

			childEntities.push_back(subEntity);
		}
	}

	else if (gltfNode.camera)
	{
		static size_t cameraId = 0;
		std::string cameraName = gltfNode.camera->name ? gltfNode.camera->name : "Camera " + std::to_string(cameraId++);

		entity = scene.CreateEntity(cameraName);
		entity.AddComponent<CameraComponent>();
	}
	else if (gltfNode.light)
	{
		static size_t lightID = 0;
		std::string lightName = gltfNode.light->name ? gltfNode.light->name : "Light " + std::to_string(lightID++);

		entity = scene.CreateEntity(lightName);
		auto& lightComponent = entity.AddComponent<LightComponent>();
		switch (gltfNode.light->type)
		{
		case cgltf_light_type_directional:
			lightComponent.Type = LightComponent::kDirectionalLight;
			lightComponent.Intensity = gltfNode.light->intensity > 0 ? (float)gltfNode.light->intensity : 9.0f;
			break;

		case cgltf_light_type_point:
			lightComponent.Type = LightComponent::kOmniLight;
			lightComponent.Intensity = gltfNode.light->intensity > 0 ? (float)gltfNode.light->intensity : 420.0f;
			break;

		case cgltf_light_type_spot:
			lightComponent.Type = LightComponent::kSpotLight;
			lightComponent.Intensity = gltfNode.light->intensity > 0 ? (float)gltfNode.light->intensity : 420.0f;
			break;

		case cgltf_light_type_invalid:
		default:
			// Ignore
			assert(false);
		}
		
		std::memcpy(
			&lightComponent.Colour.x,
			&gltfNode.light->color[0],
			sizeof(float) * 3);

		lightComponent.Range = gltfNode.light->range > 0 ? (float)gltfNode.light->range : std::numeric_limits<float>().max();
		lightComponent.InnerConeAngle = (float)gltfNode.light->spot_inner_cone_angle;
		lightComponent.OuterConeAngle = (float)gltfNode.light->spot_outer_cone_angle;
	}

	if (!entity)
	{
		static size_t emptyNode = 0;

		std::string nodeName = gltfNode.name ? gltfNode.name : "Scene Node " + std::to_string(emptyNode++);
		entity = scene.CreateEntity(nodeName);
	}

	// Create an entity map that can be used?
	auto& transform = entity.GetComponent<TransformComponent>();
	if (gltfNode.has_scale)
	{
		std::memcpy(
			&transform.LocalScale.x,
			&gltfNode.scale[0],
			sizeof(float) * 3);
		transform.SetDirty(true);
	}
	if (gltfNode.has_rotation)
	{
		std::memcpy(
			&transform.LocalRotation.x,
			&gltfNode.rotation[0],
			sizeof(float) * 4);

		transform.SetDirty(true);
	}
	if (gltfNode.has_translation)
	{
		std::memcpy(
			&transform.LocalTranslation.x,
			&gltfNode.translation[0],
			sizeof(float) * 3);

		transform.SetDirty(true);
	}
	if (gltfNode.has_matrix)
	{
		std::memcpy(
			&transform.WorldMatrix._11,
			&gltfNode.matrix[0],
			sizeof(float) * 16);
		transform.SetDirty(true);

		transform.ApplyTransform();
	}

	// GLTF default light Direciton is forward - I want this to be downwards.
	if (gltfNode.light)
	{
		transform.RotateRollPitchYaw(XMFLOAT3(-XM_PIDIV2, 0, 0));
	}

	transform.UpdateTransform();

	if (cReverseZ)
	{
		transform.LocalTranslation.z *= -1.0f;
		transform.LocalRotation.x *= -1.0f;
		transform.LocalRotation.y *= 1.0f;
		transform.SetDirty();
		transform.UpdateTransform();
	}

	for (auto& child : childEntities)
	{
		child.AttachToParent(entity, true);
	}

	if (parent)
	{
		entity.AttachToParent(parent, true);
	}

	for (int i = 0; i < gltfNode.children_count; i++)
	{
		if (gltfNode.children[i])
			this->LoadNode(*gltfNode.children[i], entity, scene);
	}
}

std::shared_ptr<Assets::Texture> GltfSceneLoader::LoadTexture(
	const cgltf_texture* cglftTexture,
	bool isSRGB,
	const cgltf_data* objects,
	CgltfContext& context,
	ICommandList* commandList)
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

	// TOD: I AM HERE
	// Create a texture cache inline for now - screw the old implementation
	auto texture = this->m_textureCache->GetTexture(name);
	if (texture)
	{
		return texture;
	}

	if (activeImage->buffer_view)
	{
		uint8_t* dataPtr = static_cast<uint8_t*>(activeImage->buffer_view->data) + activeImage->buffer_view->offset;
		const size_t dataSize = activeImage->buffer_view->size;

		for (const auto& blob : context.Blobs)
		{
			const uint8_t* blobData = static_cast<const uint8_t*>(blob->Data());
			const size_t blobSize = blob->Size();

			if (blobData < dataPtr && blobData + blobSize > dataPtr)
			{
				// Found the file blob - create a range blob out of it and keep a strong reference.
				assert(dataPtr + dataSize <= blobData + blobSize);

				// TODO: Fix me
				assert(false);
				// textureData = std::make_shared<BufferRegionBlob>(blob, dataPtr - blobData, dataSize);
				break;
			}
		}

		std::shared_ptr<Core::IBlob> textureData = CreateBlob(dataPtr, dataSize);
		texture = this->m_textureCache->LoadTexture(textureData, name, mimeType, isSRGB, commandList);
	}
	else
	{
		texture = this->m_textureCache->LoadTexture(this->m_filename.parent_path() / activeImage->uri, isSRGB, commandList);
	}

	return texture;
}

void GltfSceneLoader::LoadMaterialData(
	const cgltf_material* pMaterials,
	uint32_t materialCount,
	const cgltf_data* objects,
	CgltfContext& context,
	ICommandList* commandList,
	Scene& scene)
{
	Entity rootMaterialNode = scene.CreateEntity("Materials");
	scene.AttachToParent(rootMaterialNode, this->m_rootNode);

	for (int i = 0; i < materialCount; i++)
	{
		const auto& cgltfMtl = pMaterials[i];

		std::string name = cgltfMtl.name ? cgltfMtl.name : "Material " + std::to_string(i);
		Entity mtlEntity = scene.CreateEntity(name);
		scene.AttachToParent(mtlEntity, rootMaterialNode);

		this->m_materialEntityMap[&cgltfMtl] = mtlEntity;

		MaterialComponent& mtl = mtlEntity.AddComponent<MaterialComponent>();

		if (cgltfMtl.alpha_mode != cgltf_alpha_mode_opaque)
		{
			mtl.BlendMode = Renderer::BlendMode::Alpha;
		}

		if (cgltfMtl.has_pbr_specular_glossiness)
		{
			// // LOG_CORE_WARN("Material %s contains unsupported extension 'PBR Specular_Glossiness' workflow ");
		}
		else if (cgltfMtl.has_pbr_metallic_roughness)
		{
			mtl.BaseColourTexture = this->LoadTexture(
				cgltfMtl.pbr_metallic_roughness.base_color_texture.texture,
				true,
				objects,
				context,
				commandList);

			mtl.BaseColour = 
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
		mtl.IsDoubleSided = cgltfMtl.double_sided;
	}
}


void GltfSceneLoader::LoadMeshData(
	const cgltf_mesh* pMeshes,
	uint32_t meshCount,
	ICommandList* commandList,
	Scene& scene)
{
	Entity rootMeshNode = scene.CreateEntity("Meshes");
	scene.AttachToParent(rootMeshNode, this->m_rootNode);

	// TODO: Create the Per Mesh Vertex and index buffers.
	for (int i = 0; i < meshCount; i++)
	{
		const auto& cgltfMesh = pMeshes[i];
		std::string name = cgltfMesh.name ? cgltfMesh.name : "Mesh " + std::to_string(i);

		Entity rootModelEntity = scene.CreateEntity(name);
		scene.AttachToParent(rootModelEntity, rootMeshNode);

		for (int iPrim = 0; iPrim < cgltfMesh.primitives_count; iPrim++)
		{
			const auto& cgltfPrim = cgltfMesh.primitives[iPrim];
			if (cgltfPrim.type != cgltf_primitive_type_triangles ||
				cgltfPrim.attributes_count == 0)
			{
				continue;
			}

			Entity meshEntity = scene.CreateEntity(name + "_Prim" + std::to_string(iPrim));
			scene.AttachToParent(meshEntity, rootModelEntity);

			this->m_meshEntityMap[&cgltfMesh].push_back(meshEntity);
			auto& mesh = meshEntity.AddComponent<MeshComponent>();

			if (cgltfPrim.indices)
			{
				mesh.TotalIndices = cgltfPrim.indices->count;
			}
			else
			{
				mesh.TotalIndices = cgltfPrim.attributes->data->count;
			}

			mesh.TotalVertices = cgltfPrim.attributes->data->count;

			// Allocate Require data.
			mesh.InitializeCpuBuffers(scene.GetAllocator());

			if (cgltfPrim.indices)
			{
				assert(cgltfPrim.indices->component_type == cgltf_component_type_r_32u ||
					cgltfPrim.indices->component_type == cgltf_component_type_r_16u ||
					cgltfPrim.indices->component_type == cgltf_component_type_r_8u);
				assert(cgltfPrim.indices->type == cgltf_type_scalar);
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
					assert(cgltfAttribute.data->type == cgltf_type_vec3);
					assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
					cgltfPositionsAccessor = cgltfAttribute.data;
					break;

				case cgltf_attribute_type_tangent:
					assert(cgltfAttribute.data->type == cgltf_type_vec4);
					assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
					cgltfTangentsAccessor = cgltfAttribute.data;
					break;

				case cgltf_attribute_type_normal:
					assert(cgltfAttribute.data->type == cgltf_type_vec3);
					assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
					cgltfNormalsAccessor = cgltfAttribute.data;
					break;

				case cgltf_attribute_type_texcoord:
					if (std::strcmp(cgltfAttribute.name, "TEXCOORD_0") == 0)
					{
						assert(cgltfAttribute.data->type == cgltf_type_vec2);
						assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
						cgltfTexCoordsAccessor = cgltfAttribute.data;
					}
					break;
				}
			}

			assert(cgltfPositionsAccessor);
			if (cgltfPrim.indices)
			{
				size_t indexCount = cgltfPrim.indices->count;

				// copy the indices
				auto [indexSrc, indexStride] = CgltfBufferAccessor(cgltfPrim.indices, 0);

				uint32_t* indexDst = mesh.Indices;
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
					assert(false);
				}
			}
			else
			{
				size_t indexCount = cgltfPositionsAccessor->count;

				// generate the indices
				uint32_t* indexDst = mesh.Indices;
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
					mesh.Positions,
					positionSrc,
					positionStride * cgltfPositionsAccessor->count);
			}

			if (cgltfNormalsAccessor)
			{
				mesh.Flags |= MeshComponent::Flags::kContainsNormals;
				auto [normalSrc, normalStride] = CgltfBufferAccessor(cgltfNormalsAccessor, sizeof(float) * 3);

				// Do a mem copy?
				std::memcpy(
					mesh.Normals,
					normalSrc,
					normalStride * cgltfNormalsAccessor->count);
			}

			if (cgltfTangentsAccessor)
			{
				mesh.Flags |= MeshComponent::Flags::kContainsTangents;
				auto [tangentSrc, tangentStride] = CgltfBufferAccessor(cgltfTangentsAccessor, sizeof(float) * 4);

				// Do a mem copy?
				std::memcpy(
					mesh.Tangents,
					tangentSrc,
					tangentStride * cgltfTangentsAccessor->count);
			}

			if (cgltfTexCoordsAccessor)
			{
				mesh.Flags |= MeshComponent::Flags::kContainsTexCoords;
				assert(cgltfTexCoordsAccessor->count == cgltfPositionsAccessor->count);

				auto [texcoordSrc, texcoordStride] = CgltfBufferAccessor(cgltfTexCoordsAccessor, sizeof(float) * 2);

				std::memcpy(
					mesh.TexCoords,
					texcoordSrc,
					texcoordStride * cgltfTexCoordsAccessor->count);
			}
			else
			{
				std::memset(
					mesh.TexCoords,
					0.0f,
					cgltfPositionsAccessor->count * sizeof(float) * 2);
			}

			auto it = this->m_materialEntityMap.find(cgltfPrim.material);
			mesh.Material = it == this->m_materialEntityMap.end() ? entt::null : (entt::entity)it->second;
			assert(mesh.Material != entt::null);

			// GLTF 2.0 front face is CCW, I currently use CW as front face.
			// something to consider to change.
			if (cReverseWinding)
			{
				mesh.ReverseWinding();
			}

			// Generate Tangents
			if ((mesh.Flags & Assets::Mesh::Flags::kContainsNormals) != 0 && (mesh.Flags & Assets::Mesh::Flags::kContainsTexCoords) != 0 && (mesh.Flags & Assets::Mesh::Flags::kContainsTangents) == 0)
			{
				mesh.ComputeTangentSpace();
				mesh.Flags |= MeshComponent::Flags::kContainsTangents;
			}

			if (cReverseZ)
			{
				mesh.FlipZ();
			}

			// Calculate AABB
			mesh.ComputeBounds();

			// Calculate Meshlet Data
			mesh.ComputeMeshletData(scene.GetAllocator());

			mesh.BuildRenderData(scene.GetAllocator(), RHI::IGraphicsDevice::GPtr);
		}
	}
}
