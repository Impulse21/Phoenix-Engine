#pragma once

namespace phx::gfx
{
	enum class GfxApi
	{
		Null = 0,
		DX12,
		Vulkan,
	};

	class IGfxDevice
	{
	public:
		virtual void Initialize() = 0;

		virtual ~IGfxDevice() = default;
	};
}