#pragma once

#include <stdint.h>

#define MAKE_ID(a,b,c,d)		(((d) << 24) | ((c) << 16) | ((b) << 8) | ((a)))

namespace PhxEngine
{
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

	template<class TMetadata, class TCpuData = void>
	struct ResourceHeader
	{
		uint32_t Id;
		uint16_t Version;

		Region<TMetadata> Metadata;
		Region<CpuData> CpuData;
		GpuRegion UnstructuredGpuData;
		Array<Array<char>> StringTable;
		Array<uint32_t> Dependencies;
	};

	namespace MeshResource
	{
		struct Metadata
		{
			uint32_t VertexBufferOffset;
			uint32_t VertexBufferSize;
			uint32_t IndexBufferOffset;
			uint32_t IndexBufferSize;
			uint32_t VertexBufferStride;
			uint32_t MeshletBufferOffset;
			uint32_t MeshletBufferSize;
			uint32_t NumMeshlets;
			uint8_t IndexBufferFormat;
		};

		struct CpuData
		{
		};

		using Header = ResourceHeader<Metadata>;
	}

	namespace MaterialResource
	{
		struct Metadata
		{

		};
		struct CpuData
		{

		};
		using Header = ResourceHeader<Metadata, CpuData>;
	}

	namespace TextureResource
	{
		struct Metadata
		{

		};

		using Header = ResourceHeader<Metadata>;
	}

	template<typename ResourceType>
	struct ResourceId
	{
		static constexpr uint32_t Value = MAKE_ID('P', 'U', 'K', 'N');
	};


	template<>
	struct ResourceId<MeshResource::Header>
	{
		static constexpr uint32_t Value = MAKE_ID('P', 'M', 'S', 'H');
	};

	template<>
	struct ResourceId<MaterialResource::Header>
	{
		static constexpr uint32_t Value = MAKE_ID('P', 'M', 'T', 'L');
	};

	template<>
	struct ResourceId<TextureResource::Header>
	{
		static constexpr uint32_t Value = MAKE_ID('P', 'T', 'E', 'X');
	};

	template<typename ResourceType>
	struct ResourceVersion
	{
	};


	template<>
	struct ResourceVersion<MeshResource::Header>
	{
		static constexpr uint16_t Value = 1;
	};

	template<>
	struct ResourceId<MaterialResource::Header>
	{
		static constexpr uint16_t Value = 1;
	};

	template<>
	struct ResourceId<TextureResource::Header>
	{
		static constexpr uint16_t Value = 1;
	};
}