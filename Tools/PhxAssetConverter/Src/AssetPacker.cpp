#include "AssetPacker.h"
#if false
#include <PhxEngine/Assets/AssetFile.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Assets/Assets.h>

#include <PhxEngine/Core/Log.h>

#include <string>
#include <fstream>      // std::ofstream

using namespace PhxEngine::Assets;
namespace
{
	class MtlAssetPacker : public AssetPacker
	{
		const MaterialInfo* m_mtlInfo;
	public:
		MtlAssetPacker(const MaterialInfo* mtlInfo)
			: AssetPacker()
			, m_mtlInfo(mtlInfo)
		{
		}

		AssetFile Pack()
		{
			AssetFile file = {};
			file.ID[0] = 'M';
			file.ID[1] = 'A';
			file.ID[2] = 'T';
			file.ID[3] = 'E';
			file.Version = cCurrentAssetFileVersion;
			nlohmann::json metadata;

			std::array<float, 4> baseColourV;
			baseColourV[0] = this->m_mtlInfo->BaseColour.x;
			baseColourV[1] = this->m_mtlInfo->BaseColour.y;
			baseColourV[2] = this->m_mtlInfo->BaseColour.z;
			baseColourV[3] = this->m_mtlInfo->BaseColour.w;
			metadata["base_color"] = baseColourV;

			std::array<float, 4> emissiveV;
			emissiveV[0] = this->m_mtlInfo->Emissive.x;
			emissiveV[1] = this->m_mtlInfo->Emissive.y;
			emissiveV[2] = this->m_mtlInfo->Emissive.z;
			emissiveV[3] = this->m_mtlInfo->Emissive.w;
			metadata["emissive_color"] = emissiveV;

			metadata["metalness"] = this->m_mtlInfo->Metalness;
			metadata["roughness"] = this->m_mtlInfo->Roughness;
			metadata["ao"] = emissiveV;
			metadata["is_double_sided"] = emissiveV;

			metadata["albedo_texture"] = emissiveV;
			metadata["material_texture"] = emissiveV;
			metadata["ao_texture"] = emissiveV;
			metadata["normal_texture"] = emissiveV;
			file.Metadata = metadata.dump();

			return file;
		}

	};

	class MeshAssetPacker : public AssetPacker
	{
		enum class VertexStreamsTypes
		{
			Positions,
			TexCoords0,
			TexCoords1,
			Normals,
			Tangents,
			Colour,

			Count
		};

		struct GeometryBufferHeader
		{
			uint64_t IndexOffset : 28;
			uint64_t IndexStride : 4;
			// -- 32 bit boundry

			struct StreamInfo
			{
				uint64_t Offset : 28;
				uint64_t Stride : 4;
			} Steams[static_cast<size_t>(VertexStreamsTypes::Count)];
		};

		struct MeshletBufferHeader
		{
			uint64_t MeshletOffset			: 28;
			uint64_t MeshletStride			: 4;
			uint64_t MeshletVerticesOffset	: 28;
			uint64_t MeshletVerticesStride	: 4;
			uint64_t MeshletTrianglesOffset : 28;
			uint64_t MeshletTrianglesStride : 4;
			uint64_t MeshletBoundsOffsets	: 28;
			uint64_t MeshletBoundsStride	: 4;

		};

		const MeshInfo* m_meshInfo;

	public:
		MeshAssetPacker(const MeshInfo* meshInfo)
			: AssetPacker()
			, m_meshInfo(meshInfo)
		{
		}

		AssetFile Pack()
		{
			AssetFile file= {};
			file.ID[0] = 'M';
			file.ID[1] = 'E';
			file.ID[2] = 'S';
			file.ID[3] = 'H';
			file.Version = cCurrentAssetFileVersion;
			nlohmann::json metadata;
			metadata["num_indices"] = this->m_meshInfo->GetTotalIndices();
			metadata["num_vertices"] = this->m_meshInfo->GetTotalVertices();

			const size_t geometryBufferSize = this->BuildGeometryBufferData();
			metadata["geometry_buffer_size"] = geometryBufferSize;

			const size_t meshletBufferSize = this->BuildMeshletBufferData();
			metadata["meshet_buffer_size"] = meshletBufferSize;

			std::array<float, 4> boundingSphere;
			boundingSphere[0] = this->m_meshInfo->BoundingSphere.Centre.x;
			boundingSphere[1] = this->m_meshInfo->BoundingSphere.Centre.y;
			boundingSphere[2] = this->m_meshInfo->BoundingSphere.Centre.z;
			boundingSphere[3] = this->m_meshInfo->BoundingSphere.Radius;

			metadata["bounding_box"] = boundingSphere;

			std::array<float, 6> boundingBox;
			boundingBox[0] = this->m_meshInfo->BoundingBox.Min.x;
			boundingBox[1] = this->m_meshInfo->BoundingBox.Min.y;
			boundingBox[2] = this->m_meshInfo->BoundingBox.Min.z;
			boundingBox[3] = this->m_meshInfo->BoundingBox.Max.x;
			boundingBox[4] = this->m_meshInfo->BoundingBox.Max.y;
			boundingBox[5] = this->m_meshInfo->BoundingBox.Max.z;

			metadata["bounding_box"] = boundingBox;

			metadata["compression"] = "None";

			for (auto& meshPart : m_meshInfo->MeshPart)
			{
				nlohmann::json meshPartMetadata;
				meshPartMetadata["material_name"] = meshPart.Material.c_str();
				meshPartMetadata["index_offset"] = meshPart.IndexOffset;
				meshPartMetadata["index_count"] = meshPart.IndexCount;

				metadata["mesh_part"].push_back(meshPartMetadata);
			}

			file.Metadata = metadata.dump();

			file.UnstructuredGpuData.resize(geometryBufferSize + meshletBufferSize);
			std::memcpy(file.UnstructuredGpuData.data(), this->m_geometryBuffer.data(), geometryBufferSize);

			std::memcpy(
				file.UnstructuredGpuData.data() + geometryBufferSize,
				this->m_meshletBuffer.data(),
				meshletBufferSize);

			return file;
		}

	private:
		void WriteGpuData()
		{
			std::stringstream ss;
		}

	private:
		size_t BuildGeometryBufferData()
		{
			const size_t alignment = 16llu;

			// Calculate size in bytes
			const size_t sizeInBytes =
				AlignTo(sizeof(GeometryBufferHeader), alignment) +
				AlignTo(this->m_meshInfo->Indices.size() * sizeof(uint32_t), alignment) +
				AlignTo(this->m_meshInfo->VertexStreams.Positions.size() * sizeof(DirectX::XMFLOAT3), alignment) +
				(EnumHasAllFlags(this->m_meshInfo->VertexStreamFlags, VertexStreamFlags::kContainsTexCoords)
					? AlignTo(this->m_meshInfo->VertexStreams.TexCoords.size() * sizeof(DirectX::XMFLOAT2), alignment)
					: 0 ) +
				(EnumHasAllFlags(this->m_meshInfo->VertexStreamFlags, VertexStreamFlags::kContainsTexCoords1)
					? AlignTo(this->m_meshInfo->VertexStreams.TexCoords1.size() * sizeof(DirectX::XMFLOAT2), alignment)
					: 0) +
				(EnumHasAllFlags(this->m_meshInfo->VertexStreamFlags, VertexStreamFlags::kContainsNormals)
					? AlignTo(this->m_meshInfo->VertexStreams.Normals.size() * sizeof(DirectX::XMFLOAT3), alignment)
					: 0) +
				(EnumHasAllFlags(this->m_meshInfo->VertexStreamFlags, VertexStreamFlags::kContainsTangents)
					? AlignTo(this->m_meshInfo->VertexStreams.Tangents.size() * sizeof(DirectX::XMFLOAT4), alignment)
					: 0) +
				(EnumHasAllFlags(this->m_meshInfo->VertexStreamFlags, VertexStreamFlags::kContainsColour)
					? AlignTo(this->m_meshInfo->VertexStreams.Colour.size() * sizeof(DirectX::XMFLOAT3), alignment)
					: 0);

			this->m_geometryBuffer.resize(sizeInBytes);
			size_t offset = 0ull;

			GeometryBufferHeader* header = reinterpret_cast<GeometryBufferHeader*>(this->m_geometryBuffer.data());
			header->IndexOffset = offset;
			header->IndexStride = sizeof(uint32_t);

			offset += sizeof(GeometryBufferHeader);

			size_t dataEntrySize = this->m_meshInfo->Indices.size() * header->IndexStride;
			std::memcpy(this->m_geometryBuffer.data() + offset, this->m_meshInfo->Indices.data(), dataEntrySize);

			// -- Position ---
			offset += dataEntrySize;
			header->Steams[static_cast<size_t>(VertexStreamsTypes::Positions)].Offset = dataEntrySize;
			header->Steams[static_cast<size_t>(VertexStreamsTypes::Positions)].Stride = sizeof(float);

			dataEntrySize = this->m_meshInfo->VertexStreams.Positions.size() * sizeof(this->m_meshInfo->VertexStreams.Positions[0]);
			std::memcpy(this->m_geometryBuffer.data() + offset, this->m_meshInfo->VertexStreams.Positions.data(), dataEntrySize);

			// -- Tex Coords ---
			offset += dataEntrySize;
			header->Steams[static_cast<size_t>(VertexStreamsTypes::TexCoords0)].Offset = offset;
			header->Steams[static_cast<size_t>(VertexStreamsTypes::TexCoords0)].Stride = 0;
			if (EnumHasAllFlags(this->m_meshInfo->VertexStreamFlags, VertexStreamFlags::kContainsTexCoords))
			{
				header->Steams[static_cast<size_t>(VertexStreamsTypes::TexCoords0)].Stride = sizeof(float);

				dataEntrySize = this->m_meshInfo->VertexStreams.TexCoords.size() * sizeof(this->m_meshInfo->VertexStreams.TexCoords[0]);
				std::memcpy(this->m_geometryBuffer.data() + offset, this->m_meshInfo->VertexStreams.TexCoords.data(), dataEntrySize);
			}

			// -- Tex Coords 1 ---
			offset += dataEntrySize;
			header->Steams[static_cast<size_t>(VertexStreamsTypes::TexCoords1)].Offset = offset;
			header->Steams[static_cast<size_t>(VertexStreamsTypes::TexCoords1)].Stride = 0;
			if (EnumHasAllFlags(this->m_meshInfo->VertexStreamFlags, VertexStreamFlags::kContainsTexCoords1))
			{
				header->Steams[static_cast<size_t>(VertexStreamsTypes::TexCoords1)].Stride = sizeof(float);

				dataEntrySize = this->m_meshInfo->VertexStreams.TexCoords1.size() * sizeof(this->m_meshInfo->VertexStreams.TexCoords1[0]);
				std::memcpy(this->m_geometryBuffer.data() + offset, this->m_meshInfo->VertexStreams.TexCoords1.data(), dataEntrySize);
			}

			// -- Normals ---
			offset += dataEntrySize;
			header->Steams[static_cast<size_t>(VertexStreamsTypes::Normals)].Offset = offset;
			header->Steams[static_cast<size_t>(VertexStreamsTypes::Normals)].Stride = 0;
			if (EnumHasAllFlags(this->m_meshInfo->VertexStreamFlags, VertexStreamFlags::kContainsNormals))
			{
				header->Steams[static_cast<size_t>(VertexStreamsTypes::Normals)].Stride = sizeof(float);

				dataEntrySize = this->m_meshInfo->VertexStreams.Normals.size() * sizeof(this->m_meshInfo->VertexStreams.Normals[0]);
				std::memcpy(this->m_geometryBuffer.data() + offset, this->m_meshInfo->VertexStreams.Normals.data(), dataEntrySize);
			}

			// -- Tangets ---
			offset += dataEntrySize;
			header->Steams[static_cast<size_t>(VertexStreamsTypes::Tangents)].Offset = offset;
			header->Steams[static_cast<size_t>(VertexStreamsTypes::Tangents)].Stride = 0;
			if (EnumHasAllFlags(this->m_meshInfo->VertexStreamFlags, VertexStreamFlags::kContainsNormals))
			{
				header->Steams[static_cast<size_t>(VertexStreamsTypes::Tangents)].Stride = sizeof(float);

				dataEntrySize = this->m_meshInfo->VertexStreams.Tangents.size() * sizeof(this->m_meshInfo->VertexStreams.Tangents[0]);
				std::memcpy(this->m_geometryBuffer.data() + offset, this->m_meshInfo->VertexStreams.Tangents.data(), dataEntrySize);
			}

			// -- Colour ---
			offset += dataEntrySize;
			header->Steams[static_cast<size_t>(VertexStreamsTypes::Colour)].Offset = offset;
			header->Steams[static_cast<size_t>(VertexStreamsTypes::Colour)].Stride = 0;
			if (EnumHasAllFlags(this->m_meshInfo->VertexStreamFlags, VertexStreamFlags::kContainsNormals))
			{
				header->Steams[static_cast<size_t>(VertexStreamsTypes::Colour)].Stride = sizeof(float);

				dataEntrySize = this->m_meshInfo->VertexStreams.Colour.size() * sizeof(this->m_meshInfo->VertexStreams.Colour[0]);
				std::memcpy(this->m_geometryBuffer.data() + offset, this->m_meshInfo->VertexStreams.Colour.data(), dataEntrySize);
			}

			return sizeInBytes;
		}

		size_t BuildMeshletBufferData()
		{
			const size_t alignment = 16llu;

			// Calculate size in bytes
			const size_t sizeInBytes =
				AlignTo(sizeof(MeshletBufferHeader), alignment) +
				AlignTo(this->m_meshInfo->MeshletData.Meshlets.size() * sizeof(this->m_meshInfo->MeshletData.Meshlets.front()), alignment) +
				AlignTo(this->m_meshInfo->MeshletData.MeshletTriangles.size() * sizeof(this->m_meshInfo->MeshletData.MeshletTriangles.front()), alignment) +
				AlignTo(this->m_meshInfo->MeshletData.MeshletVertices.size() * sizeof(this->m_meshInfo->MeshletData.MeshletVertices.front()), alignment) +
				AlignTo(this->m_meshInfo->MeshletData.MeshletBounds.size() * sizeof(this->m_meshInfo->MeshletData.MeshletBounds.front()), alignment);

			this->m_meshletBuffer.resize(sizeInBytes);
			size_t offset = 0ull;

			MeshletBufferHeader* header = reinterpret_cast<MeshletBufferHeader*>(this->m_meshletBuffer.data());
			header->MeshletOffset = offset;
			header->MeshletStride = sizeof(meshopt_Meshlet);

			offset += sizeof(MeshletBufferHeader);

			size_t dataEntrySize = this->m_meshInfo->MeshletData.Meshlets.size() * sizeof(this->m_meshInfo->MeshletData.Meshlets.front());
			std::memcpy(this->m_meshletBuffer.data() + offset, this->m_meshInfo->MeshletData.Meshlets.data(), dataEntrySize);

			offset += dataEntrySize;
			header->MeshletTrianglesOffset = offset;
			header->MeshletTrianglesStride= sizeof(uint8_t);

			dataEntrySize = this->m_meshInfo->MeshletData.MeshletTriangles.size() * sizeof(this->m_meshInfo->MeshletData.MeshletTriangles.front());
			std::memcpy(this->m_meshletBuffer.data() + offset, this->m_meshInfo->MeshletData.MeshletTriangles.data(), dataEntrySize);

			offset += dataEntrySize;
			header->MeshletVerticesOffset= offset;
			header->MeshletVerticesStride = sizeof(uint32_t);
			dataEntrySize = this->m_meshInfo->MeshletData.MeshletVertices.size() * sizeof(this->m_meshInfo->MeshletData.MeshletVertices.front());
			std::memcpy(this->m_meshletBuffer.data() + offset, this->m_meshInfo->MeshletData.MeshletVertices.data(), dataEntrySize);

			offset += dataEntrySize;
			header->MeshletBoundsOffsets = offset;
			header->MeshletBoundsStride = sizeof(meshopt_Bounds);
			dataEntrySize = this->m_meshInfo->MeshletData.MeshletBounds.size() * sizeof(this->m_meshInfo->MeshletData.MeshletBounds.front());
			std::memcpy(this->m_meshletBuffer.data() + offset, this->m_meshInfo->MeshletData.MeshletBounds.data(), dataEntrySize);

			return sizeInBytes;
		}

	private:
		std::vector<uint8_t> m_geometryBuffer;
		std::vector<uint8_t> m_meshletBuffer;

	};


}

AssetFile AssetPacker::Pack(MeshInfo& meshInfo)
{
	MeshAssetPacker exporter(&meshInfo);
	return exporter.Pack();
}

PhxEngine::Assets::AssetFile AssetPacker::Pack(MaterialInfo& mtlInfo)
{
	MtlAssetPacker exporter(&mtlInfo);
	return exporter.Pack();
}

bool AssetPacker::SaveBinary(IFileSystem* fs, const char* filename, AssetFile const& assetFile)
{
	std::ofstream outStream(filename, std::ios::out | std::ios::trunc | std::ios::binary);

	if (!outStream.is_open())
	{
		PHX_LOG_ERROR("Failed to open file %s", filename);
		return false;
	}

	outStream.write(assetFile.ID, 4);
	uint32_t version = assetFile.Version;

	//version
	outStream.write((const char*)&version, sizeof(uint32_t));

	//json lenght
	uint32_t lenght = static_cast<uint32_t>(assetFile.Metadata.size());
	outStream.write((const char*)&lenght, sizeof(uint32_t));

	//blob lenght
	uint32_t bloblenght = static_cast<uint32_t>(assetFile.UnstructuredGpuData.size());
	outStream.write((const char*)&bloblenght, sizeof(uint32_t));

	//json stream
	outStream.write(assetFile.Metadata.data(), lenght);

	//Gpu Data 
	outStream.write((const char*)assetFile.UnstructuredGpuData.data(), bloblenght);

	outStream.close();
	return false;
}
#endif