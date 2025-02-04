#include "GltfImporter.h"

#include <PhxCore/Assert.h>
#include <PhxCore/StringHash.h>
#include <PhxCore/VFS.h>
#include <cgltf.h>

using namespace phx;

namespace
{
	std::pair<const uint8_t*, size_t> CgltfBufferAccessor(const cgltf_accessor* accessor, size_t defaultStride)
	{
		// TODO: sparse accessor support
		const cgltf_buffer_view* view = accessor->buffer_view;
		const uint8_t* Data = (uint8_t*)view->buffer->data + view->offset + accessor->offset;
		const size_t stride = view->stride ? view->stride : defaultStride;
		return std::make_pair(Data, stride);
	}

	void ProcessPositionData(MeshData& meshData, cgltf_attribute const& cgltfAttribute, uint32_t vertexOffset)
	{
		const int stride = cgltfAttribute.data->stride;
		const size_t vertexCount = cgltfAttribute.data->count;

		auto [vertexSrc, vertexStride] = CgltfBufferAccessor(cgltfAttribute.data, 0);

		meshData.Vertex_Positions.resize(vertexOffset + vertexCount);
		for (size_t i = 0; i < vertexCount; ++i)
		{
			meshData.Vertex_Positions[vertexOffset + i] = *reinterpret_cast<const DirectX::XMFLOAT3*>(vertexSrc + i * stride);
		}

		if (cgltfAttribute.data->is_sparse)
		{
			// TODO: I am here.
		}
	}
}
std::vector<MeshData> phx::GltfMeshImporter::Import()
{
	std::vector<MeshData> retVal(m_gltfData->meshes_count);
	for (cgltf_size iMesh = 0; iMesh < m_gltfData->meshes_count; iMesh++)
	{
		auto& meshData = retVal[iMesh];
		const cgltf_mesh& cgltfMesh = m_gltfData->meshes[iMesh];

		meshData.Geometry.resize(cgltfMesh.primitives_count);
		for (cgltf_size iPrim = 0; iPrim < cgltfMesh.primitives_count; iPrim++)
		{
			const cgltf_primitive& cgltfPrim = cgltfMesh.primitives[iPrim];
			MeshData::GeometryData& geoData = meshData.Geometry[iPrim];

			geoData.MaterialId = cgltfPrim.material->name;

			const uint32_t vertexOffset = static_cast<uint32_t>(meshData.Vertex_Positions.size());

			const size_t indexRemap[] = { 0,2,1 };

			if (cgltfPrim.indices)
			{
				// Read in the index data
				const int stride = cgltfPrim.indices->stride;
				const size_t indexCount = cgltfPrim.indices->count;
				const size_t indexOffset = meshData.Indices.size();

				meshData.Indices.resize(indexOffset + indexCount);
				geoData.IndexOffset = indexOffset;
				geoData.IndexCount = indexCount;

				auto [indexSrc, indexStride] = CgltfBufferAccessor(cgltfPrim.indices, 0);

				if (stride == 1)
				{
					for (size_t i = 0; i < indexCount; i += 3)
					{
						meshData.Indices[indexOffset + i + 0] = vertexOffset + indexSrc[i + indexRemap[0]];
						meshData.Indices[indexOffset + i + 1] = vertexOffset + indexSrc[i + indexRemap[1]];
						meshData.Indices[indexOffset + i + 2] = vertexOffset + indexSrc[i + indexRemap[2]];
					}
				}
				else if (stride == 2)
				{
					for (size_t i = 0; i < indexCount; i += 3)
					{
						meshData.Indices[indexOffset + i + 0] = vertexOffset + reinterpret_cast<const uint16_t*>(indexSrc)[i + indexRemap[0]];
						meshData.Indices[indexOffset + i + 1] = vertexOffset + reinterpret_cast<const uint16_t*>(indexSrc)[i + indexRemap[1]];
						meshData.Indices[indexOffset + i + 2] = vertexOffset + reinterpret_cast<const uint16_t*>(indexSrc)[i + indexRemap[2]];
					}
				}
				else if (stride == 4)
				{
					for (size_t i = 0; i < indexCount; i += 3)
					{
						meshData.Indices[indexOffset + i + 0] = vertexOffset + reinterpret_cast<const uint32_t*>(indexSrc)[i + indexRemap[0]];
						meshData.Indices[indexOffset + i + 1] = vertexOffset + reinterpret_cast<const uint32_t*>(indexSrc)[i + indexRemap[1]];
						meshData.Indices[indexOffset + i + 2] = vertexOffset + reinterpret_cast<const uint32_t*>(indexSrc)[i + indexRemap[2]];
					}
				}
				else
				{
					PHX_ASSERT(false && "unsupported index stride!");
				}
			}

			for (int iAttr = 0; iAttr < cgltfPrim.attributes_count; iAttr++)
			{
				const cgltf_attribute& cgltfAttribute = cgltfPrim.attributes[iAttr];
				const int stride = cgltfAttribute.data->stride;
				const size_t vertexCount = cgltfAttribute.data->count;

				// -- Auto gen indices if we have too
				if (geoData.IndexCount == 0)
				{
					const size_t indexOffset = meshData.Indices.size();
					meshData.Indices.resize(indexOffset + vertexCount);
					for (size_t vi = 0; vi < vertexCount; vi += 3)
					{
						meshData.Indices[indexOffset + vi + 0] = uint32_t(vertexOffset + vi + indexRemap[0]);
						meshData.Indices[indexOffset + vi + 1] = uint32_t(vertexOffset + vi + indexRemap[1]);
						meshData.Indices[indexOffset + vi + 2] = uint32_t(vertexOffset + vi + indexRemap[2]);
					}
					geoData.IndexOffset = (uint32_t)indexOffset;
					geoData.IndexCount = (uint32_t)vertexCount;
				}



				switch (cgltfAttribute.type)
				{
				case cgltf_attribute_type_position:
					ProcessPositionData(meshData, cgltfAttribute, vertexOffset);
					break;

				case cgltf_attribute_type_tangent:
					break;

				case cgltf_attribute_type_normal:
					break;

				case cgltf_attribute_type_texcoord:
					if (std::strcmp(cgltfAttribute.name, "TEXCOORD_0") == 0)
					{
					}
					else if (std::strcmp(cgltfAttribute.name, "TEXCOORD_1") == 0)
					{
					}
					break;

				case cgltf_attribute_type_color:
					break;
				}
			}

			if (meshData.Vertex_Normals.empty())
			{
				// TODO: Create Normals

			}
		}
	}

	return retVal;
}
