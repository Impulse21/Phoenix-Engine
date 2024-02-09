#include "GltfAssetImporter.h"

#include <cgltf.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/VirtualFileSystem.h>

using namespace PhxEngine;

namespace PhxEngine::Pipeline
{
	struct CgltfContext
	{
		Core::IFileSystem* FileSystem;
		std::vector<std::shared_ptr<Core::IBlob>> Blobs;
	};
}

namespace
{
	namespace GltfHelpers
	{
		static std::pair<const uint8_t*, size_t> CgltfBufferAccessor(const cgltf_accessor* accessor, size_t defaultStride)
		{
			// TODO: sparse accessor support
			const cgltf_buffer_view* view = accessor->buffer_view;
			const uint8_t* Data = (uint8_t*)view->buffer->data + view->offset + accessor->offset;
			const size_t stride = view->stride ? view->stride : defaultStride;
			return std::make_pair(Data, stride);
		}


		static cgltf_result CgltfReadFile(
			const struct cgltf_memory_options* memory_options,
			const struct cgltf_file_options* file_options,
			const char* path,
			cgltf_size* size,
			void** Data)
		{
			Pipeline::CgltfContext* context = (Pipeline::CgltfContext*)file_options->user_data;

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

	cgltf_result CgltfReadFile(const cgltf_memory_options* memory_options, const cgltf_file_options* file_options, const char* path, cgltf_size* size, void** Data)
	{
		Pipeline::CgltfContext* context = (Pipeline::CgltfContext*)file_options->user_data;

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
}

bool PhxEngine::Pipeline::GltfAssetImporter::Import(Core::IFileSystem* fileSystem, std::string const& filename, ImportedObjects& importedObjects)
{
	importedObjects = {};

	// Load GLF File into memory
	CgltfContext cgltfContext =
	{
		.FileSystem = fileSystem,
		.Blobs = {}
	};

	cgltf_options options = { };
	options.file.read = &CgltfReadFile;
	options.file.release = &CgltfReleaseFile;
	options.file.user_data = &cgltfContext;

	std::unique_ptr<Core::IBlob> blob = fileSystem->ReadFile(filename);
	if (!blob)
	{
		PHX_LOG_ERROR("Couldn't Read file %s", filename.c_str());
		return false;
	}

	this->m_gltfData = nullptr;
	cgltf_result res = cgltf_parse(&options, blob->Data(), blob->Size(), &this->m_gltfData);
	if (res != cgltf_result_success)
	{
		PHX_LOG_ERROR("Couldn't load glTF file %s", filename.c_str());
		return false;
	}

	res = cgltf_load_buffers(&options, this->m_gltfData, filename.c_str());
	if (res != cgltf_result_success)
	{
		PHX_LOG_ERROR("Couldn't load glTF Binary data %s", filename.c_str());
		return false;
	}

	// Import the Mesh
	importedObjects.Meshes.resize(this->m_gltfData->meshes_count);
	importedObjects.Materials.resize(this->m_gltfData->materials_count);
	importedObjects.Textures.resize(this->m_gltfData->textures_count);


	for (int i = 0; i < this->m_gltfData->textures_count; i++)
	{
		cgltf_texture* texture = (this->m_gltfData->textures + i);
		const bool success = this->ImportTexture(cgltfContext, texture, importedObjects.Textures[i]);

		this->m_textureIndexLut[texture] = i;
	}

	for (int i = 0; i < this->m_gltfData->materials_count; i++)
	{
		cgltf_material* material = (this->m_gltfData->materials + i);
		const bool success = this->ImportMaterial(material, importedObjects.Materials[i]);

		this->m_materialIndexLut[material] = i;
	}

	for (int i = 0; i < this->m_gltfData->meshes_count; i++)
	{
		cgltf_mesh* mesh = (this->m_gltfData->meshes + i);
		const bool success = this->ImportMesh(mesh, importedObjects.Meshes[i]);

		this->m_meshIndexLut[mesh] = i;
	}

	return true;
}

bool PhxEngine::Pipeline::GltfAssetImporter::ImportMesh(cgltf_mesh* gltfMesh, Pipeline::Mesh& outMesh)
{
	outMesh = {};
	outMesh.Name = gltfMesh->name;

	outMesh.MeshParts.resize(gltfMesh->primitives_count);
	for (int iPrim = 0; iPrim < gltfMesh->primitives_count; iPrim++)
	{
		const auto& cgltfPrim = gltfMesh->primitives[iPrim];
		MeshPart& meshPart = outMesh.MeshParts[iPrim];
		if (cgltfPrim.type != cgltf_primitive_type_triangles ||
			cgltfPrim.attributes_count == 0)
		{
			continue;
		}

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
		const cgltf_accessor* cgltfTexCoord2sAccessor = nullptr;

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
					cgltfTexCoord2sAccessor = cgltfAttribute.data;
				}
				break;
			}
		}

		
		meshPart.Indices.resize(cgltfPrim.indices->count);

		assert(cgltfPositionsAccessor);
		if (cgltfPrim.indices)
		{
			size_t indexCount = cgltfPrim.indices->count;

			// copy the indices
			auto [indexSrc, indexStride] = CgltfBufferAccessor(cgltfPrim.indices, 0);

			uint32_t* indexDst = meshPart.Indices.data();
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

		if (cgltfPositionsAccessor)
		{
			VertexStream& stream = meshPart.VertexStreams[static_cast<size_t>(VertexStreamType::Position)];
			stream.NumComponents = 3;

			auto [positionSrc, positionStride] = CgltfBufferAccessor(cgltfPositionsAccessor, sizeof(float) * stream.NumComponents);

			stream.Data.resize(positionStride* cgltfPositionsAccessor->count);
			std::memcpy(
				stream.Data.data(),
				positionSrc,
				positionStride * cgltfPositionsAccessor->count);
		}

		if (cgltfNormalsAccessor)
		{
			VertexStream& stream = meshPart.VertexStreams[static_cast<size_t>(VertexStreamType::Normals)];
			stream.NumComponents = 3;

			auto [normalSrc, normalStride] = CgltfBufferAccessor(cgltfNormalsAccessor, sizeof(float) * stream.NumComponents);

			stream.Data.resize(normalStride* cgltfNormalsAccessor->count);
			std::memcpy(
				stream.Data.data(),
				normalSrc,
				normalStride * cgltfNormalsAccessor->count);
		}

		if (cgltfTangentsAccessor)
		{
			VertexStream& stream = meshPart.VertexStreams[static_cast<size_t>(VertexStreamType::Normals)];
			stream.NumComponents = 4;

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
	}
	return true;
}

bool PhxEngine::Pipeline::GltfAssetImporter::ImportMaterial(cgltf_material* gltfMaterial, Pipeline::Material& outMaterial)
{
	outMaterial = {};
	outMaterial.Name = gltfMaterial->name;

	if (gltfMaterial->alpha_mode != cgltf_alpha_mode_opaque)
	{
		outMaterial.BlendMode = Material::BlendMode::Alpha;
	}

	if (gltfMaterial->has_pbr_specular_glossiness)
	{
		// // LOG_CORE_WARN("Material %s contains unsupported extension 'PBR Specular_Glossiness' workflow ");
	}
	else if (gltfMaterial->has_pbr_metallic_roughness)
	{
		if (gltfMaterial->pbr_metallic_roughness.base_color_texture.texture)
		{
			outMaterial.BaseColourTextureHandle = this->m_textureIndexLut[gltfMaterial->pbr_metallic_roughness.base_color_texture.texture];
		}

		outMaterial.BaseColour =
		{
			gltfMaterial->pbr_metallic_roughness.base_color_factor[0],
			gltfMaterial->pbr_metallic_roughness.base_color_factor[1],
			gltfMaterial->pbr_metallic_roughness.base_color_factor[2],
			gltfMaterial->pbr_metallic_roughness.base_color_factor[3]
		};

		if (gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture)
		{
			outMaterial.MetalRoughnessTextureHandle = this->m_textureIndexLut[gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture];
		}

		outMaterial.Metalness = gltfMaterial->pbr_metallic_roughness.metallic_factor;
		outMaterial.Roughness = gltfMaterial->pbr_metallic_roughness.roughness_factor;
	}

	if (gltfMaterial->normal_texture.texture)
	{
		outMaterial.NormalMapHandle = this->m_textureIndexLut[gltfMaterial->normal_texture.texture];
	}

	if (gltfMaterial->emissive_texture.texture)
	{
		outMaterial.EmissiveTextureHandle = this->m_textureIndexLut[gltfMaterial->emissive_texture.texture];
	}

	outMaterial.IsDoubleSided = gltfMaterial->double_sided;

	return true;
}

bool PhxEngine::Pipeline::GltfAssetImporter::ImportTexture(Pipeline::CgltfContext const& cgltfContext, cgltf_texture* gltfTexture, Pipeline::Texture& outTexture)
{
	if (!gltfTexture)
	{
		return true;
	}

	outTexture = {};
	outTexture.Name = gltfTexture->name;
	outTexture.DataFile = gltfTexture->image->uri;
	outTexture.MimeType = gltfTexture->image->mime_type ? gltfTexture->image->mime_type : "";

	// If this texture is emedded, load it into memory
	if (gltfTexture->image->buffer_view)
	{
		uint8_t* dataPtr = static_cast<uint8_t*>(gltfTexture->image->buffer_view->data) + gltfTexture->image->buffer_view->offset;
		const size_t dataSize = gltfTexture->image->buffer_view->size;

		for (const auto& blob : cgltfContext.Blobs)
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

		outTexture.DataBlob = Core::CreateBlob(dataPtr, dataSize);
	}

	return true;
}
