#pragma once

#include <PhxCore/Base.h>

namespace phx::rhi
{

	enum class ReferenceType : unsigned int
	{
		Invalid,
		RenderTarget,
		PassResult,
		ExternalSRV

	};

	struct Reference
	{
		enum : uint8_t
		{
			AllSubresources = 0x7f
		};

		union
		{
			struct
			{
				ReferenceType Type : 2;
				unsigned int Depth : 1;
				unsigned int EntireTexture : 1;		 // Used for PassResult types to specify AllSubresources (since SubResource is used for the output index)
				unsigned int MipSlice : 1;
				unsigned int SubResource : 7;
				unsigned int MipLevel : 4;
				unsigned int Index : 16;
			};
			unsigned int Data;
		};

		Reference() {}

		constexpr Reference(const ReferenceType type, const unsigned int index, const unsigned int sub_resource, const unsigned int depth = 0, const unsigned int entire_texture = 0, const unsigned int mip_slice = 0, const unsigned int mip_level = 0) :
			Type(type),
			Depth(depth),
			EntireTexture(entire_texture),
			MipSlice(mip_slice),
			SubResource(sub_resource),
			MipLevel(mip_level),
			Index(index)
		{
			PHX_ASSERT(sub_resource < 128);
			PHX_ASSERT(index < 65536);
			PHX_ASSERT(mip_slice == 1 || mip_level == 0);
			PHX_ASSERT(mip_level < 16);
		}

		constexpr Reference SubResourceRef(const unsigned int sub_resource) const { return Reference(Type, Index, sub_resource, Depth); }
		constexpr Reference MipSliceRef(const unsigned int mip_level) const { return Reference(Type, Index, 0, Depth, 0, 1, mip_level); }

		static constexpr Reference Null()
		{
			return Reference(ReferenceType::Invalid, 0, 0);
		}

		explicit operator bool() const { return Data != 0; }
	};
}