#pragma once

#include <stdint.h>

namespace PhxEngine::Assets
{
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
	using CpuRegion = Region<void>;
	struct AssetHeader
	{
		uint32_t id;
		GpuRegion UnstructuredGpuData;
		CpuRegion UnstructuredCpuData;
		Array<uint32_t> Dependencies;
	};

#define MAKE_ID(a,b,c,d)		(((d) << 24) | ((c) << 16) | ((b) << 8) | ((a)))
	namespace MeshFormat
	{
		constexpr uint32_t Id = MAKE_ID('P', 'M', 'S', 'H');

	}
}