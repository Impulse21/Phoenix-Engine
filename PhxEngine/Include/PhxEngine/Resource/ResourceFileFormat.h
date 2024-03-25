#pragma once

#include <stdint.h>


#define MAKE_ID(a,b,c,d)		(((d) << 24) | ((c) << 16) | ((b) << 8) | ((a)))


namespace PhxEngine
{

	constexpr uint32_t ResourceId = MAKE_ID('P', 'R', 'E', 'S');

	enum class Compression : uint16_t
	{
		None = 0,
		GDeflate
	};


	template<class T>
	class RelativePtr
	{
		using OffsetType = uint32_t;
	public:

		T* Get() { return static_cast<T*>(((char*)this) + this->m_offset); }
		const T* Get() const { return static_cast<T*>((((char*)this) + this->m_offset)); }

		void Set(T* ptr)
		{
			this->m_offset = static_cast<OffsetType>((char*)ptr - ((char*)this));
		}
		
		operator T* () { return this->Get(); }
		operator const T* () const { return this->Get(); }
		T* operator=(const T* ptr)
		{
			if (ptr)
			{
				this->Set(ptr);
			}
			else
			{
				this->m_offset = 0;
			}
		}

		uint32_t operator=(uint32_t offset)
		{
			this->m_offset = static_cast<OffsetType>(offset - ((char*)this));
		}

		operator uint32_t() { return this->m_offset; }

	private:
		OffsetType m_offset;
	};

	template<class T>
	struct Array
	{
		RelativePtr<T> Data;

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
		RelativePtr<T> Data;
		uint32_t CompressedSize;
		uint32_t UncompressedSize;
	};
	using GpuRegion = Region<void>;


	struct DependenciesHeader
	{
		Array<RelativePtr<char>> Dependencies;
	};
	using DependenciesRegion = Region<DependenciesHeader>;

	struct StringHashTableHeader
	{
		struct StringHash
		{
			uint32_t Hash;
			RelativePtr<char> Str;
		};

		Array<StringHash> StringHashTable;
	};
	using StringHashTableHeader = Region<StringHashTableHeader>;
}