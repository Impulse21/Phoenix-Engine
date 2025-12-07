#pragma once

#include <PhxRenderer/RenderPasses.h>

namespace phx
{
	class World;
}

namespace phx::gfx
{
	struct View;
	class RenderLayer
	{
	public:
		virtual ~RenderLayer() = default;

		virtual void Finalize() = 0;
		virtual void* PreRender(phx::World& world, View const& view, RenderPass renderPass) = 0;
		virtual void Render(RenderPass renderPass, void* cachedData) = 0;

	};
}