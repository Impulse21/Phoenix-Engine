#pragma once

namespace phx
{
	namespace rhi
	{
		class CommandCtx;
	}
}

namespace phx::gfx
{
	class IRenderSystem
	{
	public:
		virtual ~IRenderSystem() = default;

		virtual void* OnPreRender() = 0;
		virtual void OnRender(rhi::CommandCtx* ctx, void* cachedData) = 0;
	};
}