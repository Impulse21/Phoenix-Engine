#pragma once

#include <PhxRhi/RHICommon.h>

namespace phx::rhi
{
	template<class TDerivedCtx>
	class BaseCommnadCtx
	{
	public:

		void* GetStagingMemory(uint64_t size, uint64_t alignment = 1)
		{
			return static_cast<TDerivedCtx*>(this)->PlatformGetStagingMemory(size, alignment);
		}

		void CopyBuffer(rhi::GpuBufferHandle handle)
		{
			static_cast<TDerivedCtx*>(this)->PlatformCopyBuffer(handle);
		}

		void CopyTexture(rhi::TextureHandle handle)
		{
			static_cast<TDerivedCtx*>(this)->PlatformCopyTexture(handle);
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
	class BaseCopyCommnadCtx : public BaseCommnadCtx<TDerivedCtx>
	{
	public:
	};
}