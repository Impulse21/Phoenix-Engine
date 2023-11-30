#pragma once

#include <stdint.h>
#include <string>
#include <PhxEngine/Core/VirtualFileSystem.h>

namespace PhxEngine::Assets
{
	constexpr uint16_t cCurrentAssetFileVersion = 1u;

	enum class Compression : uint16_t
	{
		None,
		GDeflate
	};

	//
	// A pointer/offset.  On disk, this is an offset relative to the containing
	// region (or the start of the file if this Ptr is stored in the header.)
	// After the data has been loaded, the offsets are fixed up and converted
	// into typed pointers.
	//
	template<typename T>
	union Ptr
	{
		uint32_t Offset;
		T* Ptr;
	};

	//
	// An array - stored as a Ptr, with overloaded array access operators.
	//
	template<typename T>
	struct Array
	{
		Ptr<T> Data;

		T& operator[] (size_t index)
		{
			return Data.Ptr[index];
		}

		T const& operator[] (size_t index) const
		{
			return Data.Ptr[index];
		}
	};


	template <typename T>
	struct Region
	{
		Compression Compression;
		Ptr<T> Data; // On disk this is compressed, in memory it's uncompressed
		uint32_t CompressedSize;
		uint32_t UncompressedSize;
	};

	// A regions that is loaded into GPU memory
	using GPURegion = Region<void>; 

	template<class TCpuMetadata, class TCpuData>
	struct AssetFile
	{
		char ID[4];
		uint16_t Version;

		GPURegion UnstructuredGpuData;
		Region<TCpuMetadata> CpuMetadata;
		Region<TCpuData> CpuData;
	};
}

