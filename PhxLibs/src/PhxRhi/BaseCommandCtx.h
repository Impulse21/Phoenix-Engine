#pragma once

namespace phx::rhi
{
	template<class PlatformImpl>
	class BaseCommandBuffer
	{
	public:

		// Access to the native underlying command buffer object
		auto GetPlatformHandle()
			-> decltype(static_cast<PlatformImpl*>(this)
				->PlatformGetNativeHandle())
		{
			return static_cast<PlatformImpl*>(this)->PlatformGetNativeHandle();
		}
	protected:

		BaseCommandBuffer() = default;
		~BaseCommandBuffer() = default;
	};
}