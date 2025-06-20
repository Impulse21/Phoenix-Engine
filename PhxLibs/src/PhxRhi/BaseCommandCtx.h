#pragma once

#include <PhxRhi/RHICommon.h>

namespace phx::rhi
{
	template<class TDerivedCtx>
	class BaseCommnadCtx
	{
	public:
		void UpdateBuffer(GpuBufferHandle dst, size_t size, size_t alignment, std::function<void(void*)> map_func)
		{
			static_cast<TDerivedCtx*>(this)->PlatformUpdateBuffer(dst, size, alignment, map_func);
		}

		void UpdateTexture(TextureHandle dst, const SubresourceData* data)
		{
			static_cast<TDerivedCtx*>(this)->PlatformUpdateTexture(dst, data);
		}

		void CopyBuffer(GpuBufferHandle dst, uint64_t dst_offset, GpuBufferHandle src, uint64_t src_offset, uint64_t size)
		{
			static_cast<TDerivedCtx*>(this)->PlatformCopyBuffer(dst, dst_offset, src, src_offset, size);
		}

		void CopyTexture(TextureHandle dst, uint32_t dst_x, uint32_t dst_y, uint32_t dst_z, uint32_t dst_mip, uint32_t dst_slice, TextureHandle src, uint32_t src_mip, uint32_t src_slice)
		{
			static_cast<TDerivedCtx*>(this)->PlatformCopyTexture(dst, dst_x, dst_y, dst_z, dst_mip, dst_slice, src, src_mip, src_slice);
		}
#if false
		// Access to the native underlying command buffer object
		auto GetPlatformHandle()
			-> decltype(static_cast<TDerivedDevice*>(this)
				->PlatformGetNativeHandle())
		{
			return static_cast<TDerivedCtx*>(this)->PlatformGetNativeHandle();
		}
#endif
	protected:
		BaseCommnadCtx() = default;
		virtual ~BaseCommnadCtx() = default;
	};

	template<class TDerivedCtx>
	class BaseGfxCommnadCtx : public BaseCommnadCtx<TDerivedCtx>
	{
	public:
	};

	template<class TDerivedCtx>
	class BaseComputeCommnadCtx : public BaseCommnadCtx<TDerivedCtx>
	{
	public:
	};

	template<class TDerivedCtx>
	class BaseCopyCommnadCtx : public BaseCommnadCtx<TDerivedCtx>
	{
	public:
	};
}