#pragma once

namespace phx
{
	class World;
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
		inline static IRenderSystem* Ptr = nullptr;

	public:
		virtual ~IRenderSystem() = default;

		virtual void RegisterWorldCallbacks(World& world) = 0;
		virtual void Finalize() = 0;
		virtual void* OnPreRender() = 0;
		virtual void OnRender(rhi::CommandCtx* ctx, void* cachedData) = 0;
	};
}