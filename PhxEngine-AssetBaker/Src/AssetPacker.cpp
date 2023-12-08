#include "AssetPacker.h"
#include <PhxEngine/Assets/AssetFile.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Assets/StaticMesh.h>


#include <string>
#include <fstream>      // std::ofstream

using namespace PhxEngine::Assets;
namespace
{

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

		const MeshInfo* m_meshInfo;

	public:
		MeshAssetPacker(std::ostream& out, const MeshInfo* meshInfo)
			: AssetPacker(out)
			, m_meshInfo(meshInfo)
		{
		}

		AssetFile Pack()
		{
			AssetFile header= {};
			header.ID[0] = 'M';
			header.ID[1] = 'E';
			header.ID[2] = 'S';
			header.ID[3] = 'H';
			header.Version = cCurrentAssetFileVersion;

			auto [fixupHeader] = WriteStruct(this->m_out, &header, &header);

			this->BuildGeometryBufferData();
			this->BuildMeshletBufferData();
			
			// Construct GPU data

			// Construct Metadata string

			// Save

		}

	private:
		void WriteGpuData()
		{
			std::stringstream ss;
		}

	private:
		void BuildGeometryBufferData()
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
		}

		void BuildMeshletBufferData()
		{
			const size_t alignment = 16llu;

			// Calculate size in bytes
			const size_t sizeInBytes =
				AlignTo(this->m_meshInfo->MeshletData.Meshlets.size() * sizeof(this->m_meshInfo->MeshletData.Meshlets.front()), alignment) +
				AlignTo(this->m_meshInfo->MeshletData.MeshletTriangles.size() * sizeof(this->m_meshInfo->MeshletData.MeshletTriangles.front()), alignment) +
				AlignTo(this->m_meshInfo->MeshletData.MeshletVertices.size() * sizeof(this->m_meshInfo->MeshletData.MeshletVertices.front()), alignment) +
				AlignTo(this->m_meshInfo->MeshletData.MeshletBounds.size() * sizeof(this->m_meshInfo->MeshletData.MeshletBounds.front()), alignment);

			this->m_meshletBuffer.resize(sizeInBytes);
			size_t offset = 0ull;

			size_t dataEntrySize = this->m_meshInfo->MeshletData.Meshlets.size() * sizeof(this->m_meshInfo->MeshletData.Meshlets.front());
			std::memcpy(this->m_meshletBuffer.data() + offset, this->m_meshInfo->MeshletData.Meshlets.data(), dataEntrySize);

			offset += dataEntrySize;
			dataEntrySize = this->m_meshInfo->MeshletData.MeshletTriangles.size() * sizeof(this->m_meshInfo->MeshletData.MeshletTriangles.front());
			std::memcpy(this->m_meshletBuffer.data() + offset, this->m_meshInfo->MeshletData.MeshletTriangles.data(), dataEntrySize);

			offset += dataEntrySize;
			dataEntrySize = this->m_meshInfo->MeshletData.MeshletVertices.size() * sizeof(this->m_meshInfo->MeshletData.MeshletVertices.front());
			std::memcpy(this->m_meshletBuffer.data() + offset, this->m_meshInfo->MeshletData.MeshletVertices.data(), dataEntrySize);

			offset += dataEntrySize;
			dataEntrySize = this->m_meshInfo->MeshletData.MeshletBounds.size() * sizeof(this->m_meshInfo->MeshletData.MeshletBounds.front());
			std::memcpy(this->m_meshletBuffer.data() + offset, this->m_meshInfo->MeshletData.MeshletBounds.data(), dataEntrySize);
		}

	private:
		std::vector<uint8_t> m_geometryBuffer;
		std::vector<uint8_t> m_meshletBuffer;

	};
}

void AssetPacker::Pack(const char* filename, MeshInfo& meshInfo)
{
	std::ofstream outStream(filename, std::ios::out | std::ios::trunc | std::ios::binary);
	MeshAssetPacker exporter(outStream, &meshInfo);
	exporter.Pack();
	outStream.close();
}
