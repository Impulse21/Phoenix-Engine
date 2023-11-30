#include <PhxEngine/PhxEngine.h>
#include <stdint.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/Memory.h>

#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <PhxEngine/Assets/AssetFile.h>

#include "MeshBuilderData.h"

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

	void ProcessData(cgltf_material const& gltfMaterial)
	{
	}

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


	void ProcessData(cgltf_mesh const& cgltfMesh, std::string filename, MeshProcessDataOptions options = {})
	{
		MeshBuildData meshBuildData;

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
				meshBuildData.TotalIndices = cgltfPrim.indices->count;
			}
			else
			{
				meshBuildData.TotalIndices = cgltfPrim.attributes->data->count;
			}

			meshBuildData.TotalVertices = cgltfPrim.attributes->data->count;

			// Allocate Require data.
			meshBuildData.Positions = reinterpret_cast<DirectX::XMFLOAT3*>(allocator->Allocate(sizeof(float) * 3 * meshBuildData.TotalVertices, 0));
			meshBuildData.TexCoords = reinterpret_cast<DirectX::XMFLOAT2*>(allocator->Allocate(sizeof(float) * 2 * meshBuildData.TotalVertices, 0));
			meshBuildData.Normals = reinterpret_cast<DirectX::XMFLOAT3*>(allocator->Allocate(sizeof(float) * 3 * meshBuildData.TotalVertices, 0));
			meshBuildData.Tangents = reinterpret_cast<DirectX::XMFLOAT4*>(allocator->Allocate(sizeof(float) * 4 * meshBuildData.TotalVertices, 0));
			meshBuildData.Colour = reinterpret_cast<DirectX::XMFLOAT3*>(allocator->Allocate(sizeof(float) * 3 * meshBuildData.TotalVertices, 0));
			meshBuildData.Indices = reinterpret_cast<uint32_t*>(allocator->Allocate(sizeof(uint32_t) * meshBuildData.TotalIndices, 0));

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

				uint32_t* indexDst = meshBuildData.Indices;
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
				uint32_t* indexDst = meshBuildData.Indices;
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
					meshBuildData.Positions,
					positionSrc,
					positionStride * cgltfPositionsAccessor->count);
			}

			if (cgltfNormalsAccessor)
			{
				meshBuildData.VertexFlags |= MeshVertexFlags::kContainsNormals;
				auto [normalSrc, normalStride] = CgltfBufferAccessor(cgltfNormalsAccessor, sizeof(float) * 3);

				// Do a mem copy?
				std::memcpy(
					meshBuildData.Normals,
					normalSrc,
					normalStride * cgltfNormalsAccessor->count);
			}

			if (cgltfTangentsAccessor)
			{
				meshBuildData.VertexFlags |= MeshVertexFlags::kContainsTangents;
				auto [tangentSrc, tangentStride] = CgltfBufferAccessor(cgltfTangentsAccessor, sizeof(float) * 4);

				// Do a mem copy?
				std::memcpy(
					meshBuildData.Tangents,
					tangentSrc,
					tangentStride * cgltfTangentsAccessor->count);
			}

			if (cgltfTexCoordsAccessor)
			{
				meshBuildData.VertexFlags |= MeshVertexFlags::kContainsTexCoords;
				assert(cgltfTexCoordsAccessor->count == cgltfPositionsAccessor->count);

				auto [texcoordSrc, texcoordStride] = CgltfBufferAccessor(cgltfTexCoordsAccessor, sizeof(float) * 2);

				std::memcpy(
					meshBuildData.TexCoords,
					texcoordSrc,
					texcoordStride * cgltfTexCoordsAccessor->count);
			}
			else
			{
				std::memset(
					meshBuildData.TexCoords,
					0.0f,
					cgltfPositionsAccessor->count * sizeof(float) * 2);
			}


			// GLTF 2.0 front face is CCW, I currently use CW as front face.
			// something to consider to change.
			if (options.ReverseWinding)
			{
				for (size_t i = 0; i < meshBuildData.TotalIndices; i += 3)
				{
					std::swap(meshBuildData.Indices[i + 1], meshBuildData.Indices[i + 2]);
				}
			}

			// Generate Tangents
			if (EnumHasAllFlags(meshBuildData.VertexFlags, (MeshVertexFlags::kContainsNormals | MeshVertexFlags::kContainsTexCoords | MeshVertexFlags::kContainsTangents)))
			{
				meshBuildData.ComputeTangentSpace();
				meshBuildData.VertexFlags |= MeshVertexFlags::kContainsTangents;
			}

			if (options.ReverseZ)
			{
				// Flip Z
				for (int i = 0; i < meshBuildData.TotalVertices; i++)
				{
					meshBuildData.Positions[i].z *= -1.0f;
				}
				for (int i = 0; i < meshBuildData.TotalVertices; i++)
				{
					meshBuildData.Normals[i].z *= -1.0f;
				}
				for (int i = 0; i < meshBuildData.TotalVertices; i++)
				{
					meshBuildData.Tangents[i].z *= -1.0f;
				}
			}

			// Calculate AABB
			DirectX::XMFLOAT3 minBounds = DirectX::XMFLOAT3(Math::cMaxFloat, Math::cMaxFloat, Math::cMaxFloat);
			DirectX::XMFLOAT3 maxBounds = DirectX::XMFLOAT3(Math::cMinFloat, Math::cMinFloat, Math::cMinFloat);

			if (meshBuildData.Positions)
			{
				for (int i = 0; i < meshBuildData.TotalVertices; i++)
				{
					minBounds = Math::Min(minBounds, meshBuildData.Positions[i]);
					maxBounds = Math::Max(maxBounds, meshBuildData.Positions[i]);
				}
			}

			meshBuildData.Aabb = AABB(minBounds, maxBounds);
			meshBuildData.BoundingSphere = Sphere(minBounds, maxBounds);


			mesh.BuildRenderData(scene.GetAllocator(), RHI::IGraphicsDevice::GPtr);
		}
	}

	void ProcessData(cgltf_texture const& gltfTextures)
	{

	}

	class AssetFileExporter
	{
	public:
		static void Export(IFileSystem* fs, std::string const& filename);
	};
}

int main(int argc, const char** argv)
{
    Core::Log::Initialize();
	CommandLineArgs::Initialize();

	std::string gltfInput;
	CommandLineArgs::GetString("input", gltfInput);

	std::string outputDirectory;
	CommandLineArgs::GetString("output_dir", outputDirectory);

	PHX_LOG_INFO("Baking Assets from %s to %s", gltfInput.c_str(), outputDirectory.c_str());

	std::unique_ptr<Core::IFileSystem> fileSystem = CreateNativeFileSystem();
	
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

	// Dispatch Async work to construct data
	tf::Executor executor;
	tf::Taskflow taskflow;

	taskflow.for_each_index(0, static_cast<int>(objects->meshes_count), 1,
		[objects](int i)
		{
			ProcessData(
				objects->meshes[i],
				objects->meshes[i].name 
					? objects->meshes[i].name 
					: "Mesh " + std::to_string(i));
		});

	taskflow.for_each_index(0, static_cast<int>(objects->materials_count), 1,
		[objects](int i)
		{
			ProcessData(objects->materials[i]);
		});

	taskflow.for_each_index(0, static_cast<int>(objects->textures_count), 1,
		[objects](int i)
		{
			ProcessData(objects->textures[i]);
		});

	executor.run(std::move(taskflow));

    return 0;
}
