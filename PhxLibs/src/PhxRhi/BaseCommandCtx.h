#pragma once

namespace phx::rhi
{
	template<class TDerivedDevice>
	class BaseCommandBuffer
	{
	public:

		// Access to the native underlying command buffer object
		auto GetPlatformHandle()
			-> decltype(static_cast<TDerivedDevice*>(this)
				->PlatformGetNativeHandle())
		{
			return static_cast<TDerivedDevice*>(this)->PlatformGetNativeHandle();
		}
	protected:

		BaseCommandBuffer() = default;
		~BaseCommandBuffer() = default;
	};
}