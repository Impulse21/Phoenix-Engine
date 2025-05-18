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
	class IRenderSubSystem
	{
	public:
		virtual ~IRenderSubSystem() = default;

		virtual void Finalize() = 0;
		virtual void* OnPreRender() = 0;
		virtual void OnRender(rhi::CommandCtx* ctx, void* cachedData) = 0;

	};
}