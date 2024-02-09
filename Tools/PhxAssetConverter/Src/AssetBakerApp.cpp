#define NOMINMAX 
#include <PhxEngine/PhxEngine.h>
#include <PhxEngine/Core/StringHashTable.h>

#if 0
#include <stdint.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Core/StopWatch.h>
#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>

#include "ModelInfoBuilder.h"
#include "AssetPacker.h"

#include <deque>
#include <mutex>
#include <functional>

#else
#include <assert.h>

#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/VirtualFileSystem.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "ArchExporter.h"
#include <PhxEngine/Assets/AssetFormats.h>
#endif

using namespace PhxEngine;

namespace
{
	constexpr const char* EmptyString = "";
}

namespace GltfHelpers
{
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
				// // LOG_CORE_WARN("Failed to parse the DDS glTF extension '%s'", ext.data);
				return nullptr;
			}

			if (tokens[0].type != JSMN_OBJECT)
			{
				// // LOG_CORE_WARN("Failed to parse the DDS glTF extension '%s'", ext.data);
				return nullptr;
			}

			for (int k = 1; k < numTokens; k++)
			{
				if (tokens[k].type != JSMN_STRING)
				{
					// // LOG_CORE_WARN("Failed to parse the DDS glTF extension '%s'", ext.data);
					return nullptr;
				}

				if (cgltf_json_strcmp(tokens + k, (const uint8_t*)ext.data, "source") == 0)
				{
					++k;
					int index = cgltf_json_to_int(tokens + k, (const uint8_t*)ext.data);
					if (index < 0)
					{
						// // LOG_CORE_WARN("Failed to parse the DDS glTF extension '%s'", ext.data);
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
		std::shared_ptr<Core::IFileSystem> FileSystem;
		std::vector<std::shared_ptr<Core::IBlob>> Blobs;
	};

	static cgltf_result CgltfReadFile(
		const struct cgltf_memory_options* memory_options,
		const struct cgltf_file_options* file_options,
		const char* path,
		cgltf_size* size,
		void** Data)
	{
		CgltfContext* context = (CgltfContext*)file_options->user_data;

		std::unique_ptr<Core::IBlob> dataBlob = context->FileSystem->ReadFile(path);
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
}

namespace
{
	struct CgltfContext
	{
		Core::IFileSystem* FileSystem;
		std::vector<std::shared_ptr<Core::IBlob>> Blobs;
	};

	cgltf_result CgltfReadFile(const cgltf_memory_options* memory_options, const cgltf_file_options* file_options, const char* path, cgltf_size* size, void** Data)
	{
		CgltfContext* context = (CgltfContext*)file_options->user_data;

		std::unique_ptr<Core::IBlob> dataBlob = context->FileSystem->ReadFile(path);
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

	static void CgltfReleaseFile(
		const struct cgltf_memory_options*,
		const struct cgltf_file_options*,
		void*)
	{
		// do nothing
	}
	static std::pair<const uint8_t*, size_t> CgltfBufferAccessor(const cgltf_accessor* accessor, size_t defaultStride)
	{
		// TODO: sparse accessor support
		const cgltf_buffer_view* view = accessor->buffer_view;
		const uint8_t* Data = (uint8_t*)view->buffer->data + view->offset + accessor->offset;
		const size_t stride = view->stride ? view->stride : defaultStride;
		return std::make_pair(Data, stride);
	}

#if false
	struct MeshProcessDataOptions
	{
		bool ReverseWinding = true;
		bool ReverseZ = true;
		std::array<size_t, 3> IndexRemap = { 0,2,1 };

		// Nvida Recomended
		// https://github.com/zeux/meshoptimizer
		size_t MaxVertices = 64;
		size_t MaxTriangles = 124;
		float ConeWeight = 0.0f;
	};

	template<typename _TKey>
	class NameRegistry
	{
	public:
		void Add(_TKey key, std::string const& name)
		{
			std::scoped_lock _(this->m_lock);
			
			this->m_registry[key] = name;
		}

		const std::string& GetName(_TKey key)
		{
			if (key == nullptr)
			{
				return EmptyString;
			}

			std::scoped_lock _(this->m_lock);
			auto itr = this->m_registry.find(key);

			if (itr == this->m_registry.end())
			{
				return EmptyString;
			}

			return itr->second;
		}

	private:
	
		std::mutex m_lock;
		std::unordered_map<_TKey, std::string> m_registry;
	};

	class GltfRegistry
	{
	public:
		void SetName(const cgltf_material* key, std::string const& name)
		{
			return this->m_materialNameRegistry.Add(key, name);
		}

		void SetName(const cgltf_mesh* key, std::string const& name)
		{
			return this->m_meshNameRegistry.Add(key, name);
		}

		void SetName(const cgltf_texture* key, std::string const& name)
		{
			return this->m_textureNameRegistry.Add(key, name);
		}

		std::string_view GetName(const cgltf_material* key)
		{
			return this->m_materialNameRegistry.GetName(key);
		}

		std::string_view GetName(const cgltf_mesh* key)
		{
			return this->m_meshNameRegistry.GetName(key);
		}

		std::string_view GetName(const cgltf_texture* key)
		{
			return this->m_textureNameRegistry.GetName(key);
		}

	private:
		NameRegistry<const cgltf_material*> m_materialNameRegistry;
		NameRegistry<const cgltf_mesh*> m_meshNameRegistry;
		NameRegistry<const cgltf_texture*> m_textureNameRegistry;
	};

	class GltfAssetFixUpQueue
	{
	public:
		using FixupCallback = std::function<void(GltfRegistry const& registry)>;
		void Enqeue(FixupCallback&& callback)
		{
			std::scoped_lock _(this->m_lock);
			this->m_queuedFuncs.emplace_back(callback);
		}

		void Process(GltfRegistry const& registry)
		{
			std::scoped_lock _(this->m_lock);
			while (!this->m_queuedFuncs.empty())
			{
				auto& func = this->m_queuedFuncs.front();
				func(registry);
				this->m_queuedFuncs.pop_front();
			}
		}

	private:
		std::mutex m_lock;
		std::deque<FixupCallback> m_queuedFuncs;
	};

	constexpr std::string BuildMeshNameString(const char* name, int index)
	{
		return name ? name : "Mesh_" + std::to_string(index);
	}

	constexpr std::string BuildMtlNameString(const char* name, int index)
	{
		return name ? name : "Material_" + std::to_string(index);
	}

	constexpr std::string BuildTextureNameString(const char* name, int index)
	{
		return name ? name : "Texture_" + std::to_string(index);
	}

	void ProcessData(cgltf_data* objects, cgltf_material const& gltfMaterial, GltfRegistry& registry, std::filesystem::path outputDir)
	{
		MaterialInfo mtlInfo(registry.GetName(&gltfMaterial));

		if (gltfMaterial.has_pbr_specular_glossiness)
		{
			LOG_WARN("Material %s contains unsupported extension 'PBR Specular_Glossiness' workflow ", mtlInfo.Name);
		}
		else if (gltfMaterial.has_pbr_metallic_roughness)
		{
			mtlInfo.BaseColourTexture = registry.GetName(gltfMaterial.pbr_metallic_roughness.base_color_texture.texture);
			mtlInfo.BaseColour =
			{
				gltfMaterial.pbr_metallic_roughness.base_color_factor[0],
				gltfMaterial.pbr_metallic_roughness.base_color_factor[1],
				gltfMaterial.pbr_metallic_roughness.base_color_factor[2],
				gltfMaterial.pbr_metallic_roughness.base_color_factor[3]
			};

			mtlInfo.MetalRoughnessTexture = registry.GetName(gltfMaterial.pbr_metallic_roughness.metallic_roughness_texture.texture);

			mtlInfo.Metalness = gltfMaterial.pbr_metallic_roughness.metallic_factor;
			mtlInfo.Roughness = gltfMaterial.pbr_metallic_roughness.roughness_factor;
		}

		// Load Normal map
		mtlInfo.NormalMapTexture = registry.GetName(gltfMaterial.normal_texture.texture);
		mtlInfo.IsDoubleSided = gltfMaterial.double_sided;

		PhxEngine::Assets::AssetFile file = AssetPacker::Pack(mtlInfo);

		std::string filename = std::string(mtlInfo.Name) + ".phxasset";
		std::filesystem::path outputFilename = (outputDir / filename);
		AssetPacker::SaveBinary(nullptr, outputFilename.generic_string().c_str(), file);
	}

	void ProcessData(cgltf_data* objects, cgltf_mesh const& cgltfMesh, GltfRegistry& registry, std::filesystem::path outputDir, MeshProcessDataOptions options = {})
	{
		MeshInfo meshInfo(registry.GetName(&cgltfMesh));

		for (int iPrim = 0; iPrim < cgltfMesh.primitives_count; iPrim++)
		{
			const auto& cgltfPrim = cgltfMesh.primitives[iPrim];
			if (cgltfPrim.type != cgltf_primitive_type_triangles ||
				cgltfPrim.attributes_count == 0)
			{
				continue;
			}

			MeshPartInfo& partInfo = meshInfo.MeshPart.emplace_back();
			if (cgltfPrim.indices)
			{
				partInfo.IndexCount = cgltfPrim.indices->count;
			}
			else
			{
				partInfo.IndexCount = cgltfPrim.attributes->data->count;
			}

			partInfo.IndexOffset = meshInfo.Indices.size();
			const size_t vertexOffset = meshInfo.VertexStreams.Positions.size();

			// Allocate Require data.
			const size_t totalVertices = cgltfPrim.attributes->data->count;
			meshInfo.AddVertexStreamSpace(totalVertices);
			meshInfo.AddIndexSpace(partInfo.IndexCount);

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
			const cgltf_accessor* cgltfTexCoordsAccessor1 = nullptr;

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
					else if (std::strcmp(cgltfAttribute.name, "TEXCOORD_1") == 0)
					{
						assert(cgltfAttribute.data->type == cgltf_type_vec2);
						assert(cgltfAttribute.data->component_type == cgltf_component_type_r_32f);
						cgltfTexCoordsAccessor1 = cgltfAttribute.data;
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

				uint32_t* indexDst = &meshInfo.Indices[partInfo.IndexOffset];
				switch (cgltfPrim.indices->component_type)
				{
				case cgltf_component_type_r_8u:
					if (!indexStride)
					{
						indexStride = sizeof(uint8_t);
					}

					for (size_t iIdx = 0; iIdx < indexCount; iIdx+=3)
					{
						indexDst[partInfo.IndexOffset + iIdx + 0] =  vertexOffset + indexSrc[iIdx + options.IndexRemap[0]];
						indexDst[partInfo.IndexOffset + iIdx + 0] = vertexOffset + indexSrc[iIdx + options.IndexRemap[1]];
						indexDst[partInfo.IndexOffset + iIdx + 0] = vertexOffset + indexSrc[iIdx + options.IndexRemap[2]];
					}
					break;

				case cgltf_component_type_r_16u:
					if (!indexStride)
					{
						indexStride = sizeof(uint16_t);
					}

					for (size_t iIdx = 0; iIdx < indexCount; iIdx += 3)
					{
						indexDst[partInfo.IndexOffset + iIdx + 0] = vertexOffset + ((uint16_t*)indexSrc)[iIdx + options.IndexRemap[0]];
						indexDst[partInfo.IndexOffset + iIdx + 0] = vertexOffset + ((uint16_t*)indexSrc)[iIdx + options.IndexRemap[1]];
						indexDst[partInfo.IndexOffset + iIdx + 0] = vertexOffset + ((uint16_t*)indexSrc)[iIdx + options.IndexRemap[2]];

					}
					break;

				case cgltf_component_type_r_32u:
					if (!indexStride)
					{
						indexStride = sizeof(uint32_t);
					}

					for (size_t iIdx = 0; iIdx < indexCount; iIdx += 3)
					{
						indexDst[partInfo.IndexOffset + iIdx + 0] = vertexOffset + ((uint32_t*)indexSrc)[iIdx + options.IndexRemap[0]];
						indexDst[partInfo.IndexOffset + iIdx + 0] = vertexOffset + ((uint32_t*)indexSrc)[iIdx + options.IndexRemap[1]];
						indexDst[partInfo.IndexOffset + iIdx + 0] = vertexOffset + ((uint32_t*)indexSrc)[iIdx + options.IndexRemap[2]];

					}
					break;

				default:
					assert(false);
				}
			}
			else
			{
				// Autogen indices:
				//	Note: this is not common, so it is simpler to create a dummy index buffer here than rewrite engine to support this case
				for (size_t vi = 0; vi < partInfo.IndexCount; vi += 3)
				{
					meshInfo.Indices[partInfo.IndexOffset + vi + 0] = uint32_t(vertexOffset + vi + options.IndexRemap[0]);
					meshInfo.Indices[partInfo.IndexOffset + vi + 1] = uint32_t(vertexOffset + vi + options.IndexRemap[1]);
					meshInfo.Indices[partInfo.IndexOffset + vi + 2] = uint32_t(vertexOffset + vi + options.IndexRemap[2]);
				}
			}

			// Process Position data
			if (cgltfPositionsAccessor)
			{
				auto [positionSrc, positionStride] = CgltfBufferAccessor(cgltfPositionsAccessor, sizeof(float) * 3);

				// Do a mem copy?
				std::memcpy(
					&meshInfo.VertexStreams.Positions[vertexOffset],
					positionSrc,
					positionStride * cgltfPositionsAccessor->count);
			}

			if (cgltfNormalsAccessor)
			{
				auto [normalSrc, normalStride] = CgltfBufferAccessor(cgltfNormalsAccessor, sizeof(float) * 3);
				meshInfo.VertexStreamFlags &= VertexStreamFlags::kContainsNormals;
				// Do a mem copy?
				std::memcpy(
					&meshInfo.VertexStreams.Normals[vertexOffset],
					normalSrc,
					normalStride* cgltfNormalsAccessor->count);
			}

			if (cgltfTangentsAccessor)
			{
				auto [tangentSrc, tangentStride] = CgltfBufferAccessor(cgltfTangentsAccessor, sizeof(float) * 4);
				meshInfo.VertexStreamFlags &= VertexStreamFlags::kContainsTangents;

				// Do a mem copy?
				std::memcpy(
					&meshInfo.VertexStreams.Tangents[vertexOffset],
					tangentSrc,
					tangentStride * cgltfTangentsAccessor->count);
				// Do a mem copy?
			}

			if (cgltfTexCoordsAccessor)
			{
				assert(cgltfTexCoordsAccessor->count == cgltfPositionsAccessor->count);
				meshInfo.VertexStreamFlags &= VertexStreamFlags::kContainsTexCoords;

				auto [texcoordSrc, texcoordStride] = CgltfBufferAccessor(cgltfTexCoordsAccessor, sizeof(float) * 2);

				// Do a mem copy?
				std::memcpy(
					&meshInfo.VertexStreams.TexCoords[vertexOffset],
					texcoordSrc,
					texcoordStride* cgltfTexCoordsAccessor->count);
			}


			if (cgltfTexCoordsAccessor1)
			{
				assert(cgltfTexCoordsAccessor1->count == cgltfTexCoordsAccessor1->count);
				meshInfo.VertexStreamFlags &= VertexStreamFlags::kContainsTexCoords1;

				auto [texcoordSrc, texcoordStride] = CgltfBufferAccessor(cgltfTexCoordsAccessor1, sizeof(float) * 2);

				// Do a mem copy?
				std::memcpy(
					&meshInfo.VertexStreams.TexCoords[vertexOffset],
					texcoordSrc,
					texcoordStride * cgltfTexCoordsAccessor->count);
			}

			partInfo.Material = cgltfPrim.material->name;

		}

		meshInfo.ComputeTangentSpace();

		if (options.ReverseZ)
		{
			meshInfo.ReverseZ();
		}

		meshInfo.OptmizeMesh()
				.GenerateMeshletData(options.MaxVertices, options.MaxTriangles, options.ConeWeight)
				.ComputeBounds();

		PhxEngine::Assets::AssetFile file = AssetPacker::Pack(meshInfo);

		std::string filename = std::string(meshInfo.Name) + ".phxasset";
		std::filesystem::path outputFilename = (outputDir / filename);
		AssetPacker::SaveBinary(nullptr, outputFilename.generic_string().c_str(), file);
	}

	void ProcessData(cgltf_data* objects, cgltf_texture const& gltfTextures, GltfRegistry& registry)
	{

	}
#endif
}

int main(int argc, const char** argv)
{
    Core::Log::Initialize();
	Core::CommandLineArgs::Initialize();
	std::string gltfInput;
	Core::CommandLineArgs::GetString("input", gltfInput);


	std::string outputDirectory;
	Core::CommandLineArgs::GetString("output_dir", outputDirectory);

	PHX_LOG_INFO("Baking Assets from %s to %s", gltfInput.c_str(), outputDirectory.c_str());
	Core::StringHashTable::Import(StringHashTableFilePath);
	std::unique_ptr<Core::IFileSystem> fileSystem = Core::CreateNativeFileSystem();
	
	// Load GLF File into memory
	CgltfContext cgltfContext =
	{
		.FileSystem = fileSystem.get(),
		.Blobs = {}
	};

	cgltf_options options = { };
	options.file.read = &CgltfReadFile;
	options.file.release = &CgltfReleaseFile;
	options.file.user_data = &cgltfContext;

	std::unique_ptr<Core::IBlob> blob = fileSystem->ReadFile(gltfInput);
	if (!blob)
	{
		PHX_LOG_ERROR("Couldn't Read file %s", gltfInput.c_str());
		return false;
	}

	cgltf_data* objects = nullptr;
	cgltf_result res = cgltf_parse(&options, blob->Data(), blob->Size(), &objects);
	if (res != cgltf_result_success)
	{
		PHX_LOG_ERROR("Couldn't load glTF file %s", gltfInput.c_str());
		return false;
	}

	res = cgltf_load_buffers(&options, objects, gltfInput.c_str());
	if (res != cgltf_result_success)
	{
		PHX_LOG_ERROR("Couldn't load glTF Binary data %s", gltfInput.c_str());
		return false;
	}

#if false
	// Dispatch Async work to construct data
	tf::Executor executor;
	tf::Taskflow taskflow;

	GltfRegistry registry;

	tf::Task registryMeshNames = taskflow.for_each_index(0, static_cast<int>(objects->meshes_count), 1,
		[objects, &registry](int i)
		{
			registry.SetName(&objects->meshes[i], BuildMeshNameString(objects->meshes[i].name, i));
		});

	tf::Task registryMtlNames = taskflow.for_each_index(0, static_cast<int>(objects->materials_count), 1,
		[objects, &registry](int i)
		{
			registry.SetName(&objects->materials[i], BuildMtlNameString(objects->materials[i].name, i));
		});

	tf::Task registryTextureNames = taskflow.for_each_index(0, static_cast<int>(objects->textures_count), 1,
		[objects, &registry](int i)
		{
			registry.SetName(&objects->textures[i], BuildTextureNameString(objects->textures[i].name, i));
		});

	tf::Task meshTask = taskflow.for_each_index(0, static_cast<int>(objects->meshes_count), 1,
		[objects, &registry, outputDirectory](int i)
		{
			PHX_LOG_INFO("Processing Mesh '%s'", objects->meshes[i].name);
			PhxEngine::Core::StopWatch timer;
			ProcessData(
				objects,
				objects->meshes[i],
				registry,
				outputDirectory);
			PhxEngine::Core::TimeStep elapsedTime = timer.Elapsed();
			PHX_LOG_INFO("Proccessing Mesh %s took %f s [%f ms]", objects->meshes[i].name, elapsedTime.GetSeconds(), elapsedTime.GetMilliseconds());
		});

	tf::Task mtlTask = taskflow.for_each_index(0, static_cast<int>(objects->materials_count), 1,
		[objects, &registry, outputDirectory](int i)
		{
			PHX_LOG_INFO("Processing Materials '%s'", objects->materials[i].name);
			PhxEngine::Core::StopWatch timer;
			ProcessData(
				objects,
				objects->materials[i],
				registry,
				outputDirectory);
			PhxEngine::Core::TimeStep elapsedTime = timer.Elapsed();
			PHX_LOG_INFO("Proccessing Materials '%s' took %f s [%f ms]", objects->materials[i].name, elapsedTime.GetSeconds(), elapsedTime.GetMilliseconds());
		});

	tf::Task textureTask = taskflow.for_each_index(0, static_cast<int>(objects->textures_count), 1,
		[objects, &registry](int i)
		{
			PHX_LOG_INFO("Processing Texture '%s'", objects->textures[i].name);
			PhxEngine::Core::StopWatch timer;
			ProcessData(
				objects,
				objects->textures[i],
				registry);
			PhxEngine::Core::TimeStep elapsedTime = timer.Elapsed();
			PHX_LOG_INFO("Proccessing Texture '%s' took %f s [%f ms]", objects->textures[i].name, elapsedTime.GetSeconds(), elapsedTime.GetMilliseconds());
		});

	meshTask.succeed(registryMeshNames, registryMtlNames);
	mtlTask.succeed(registryMtlNames, registryTextureNames);
	textureTask.succeed(registryTextureNames);

	executor.run(std::move(taskflow));
	executor.wait_for_all();
#endif
	std::filesystem::path inputPath(gltfInput);
	std::filesystem::path outputPath(outputDirectory);
	std::filesystem::path filename = inputPath.stem().generic_string() + ".ppkt";
	std::filesystem::path outputFilename = (outputDirectory / filename);
	std::ofstream outStream(outputFilename.generic_string().c_str(), std::ios::out | std::ios::trunc | std::ios::binary);

	if (!outStream.is_open())
	{
		PHX_LOG_ERROR("Failed to open file %s", filename);
		return false;
	}

	ArchGltfExporter gltfExporter(outStream, objects);
	gltfExporter.Export();
	outStream.close();

	Core::StringHashTable::Export(StringHashTableFilePath);
    return 0;
}
