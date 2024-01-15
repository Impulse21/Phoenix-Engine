#pragma once

#include <stdint.h>

namespace PhxEngine::Assets::PhxArch
{
	constexpr uint16_t kCurrentFileFormat = 1u;

	enum class Compression : uint16_t
	{
		None = 0,
		GDeflate
	};

	template<class T>
	union Ptr
	{
		uint32_t Offset;
		T* Ptr;
	};

	template<class T>
	struct Array
	{
		Ptr<T> Data;

		T& operator[](size_t index)
		{
			return Data.Ptr[index];
		}
		
		T const& operator[](size_t index) const
		{
			return Data.Ptr[index];
		}
	};

	// Region is a part of the file that can be read with a simple DS request
	template<class T>
	struct Region
	{
		Compression Compression;
		Ptr<T> Data;
		uint32_t CompressedSize;
		uint32_t UncompressedSize;
	};

	using GpuRegion = Region<void>;

	// The header is the fixed size part of the file format that references to the three main regions
	struct Header
	{
		char Id[4]; // Arch
		uint16_t Version;
		
		GpuRegion UnstructuredGpuData;
		Region<struct CpuMetadataHeader> CpuMetadata;
		Region<struct CpuData> CpuData;
	};

	struct AssetMetadata
	{
		char Id[4];
		Ptr<char> Name;
		Array<Ptr<char>> Dependencies;
	};

	struct CpuMetadataHeader
	{
		uint32_t NumPackedAssets;
		Array<AssetMetadata> AssetMetadata;
	};
}