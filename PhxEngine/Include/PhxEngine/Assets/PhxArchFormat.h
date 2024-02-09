#pragma once

#include <stdint.h>

namespace PhxEngine::Assets::PhxArch
{
	constexpr uint16_t kCurrentFileFormat = 1u;

	// Make ID
	#define MAKE_ID(a,b,c,d)		(((d) << 24) | ((c) << 16) | ((b) << 8) | ((a)))

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
		uint32_t Id;
		uint16_t Version;		
		Region<struct AssetEntriesHeader> AssetMetadata;
	};

	struct AssetEntriesHeader
	{
		uint16_t NumEntries;
		Array<struct AssetEntry> Entries;
	};

	struct AssetEntry
	{
		uint32_t Id;
		uint32_t Name;
		Array<uint32_t> Dependencies;
		GpuRegion GpuData;
		Array<char> Metadata;
	};

}