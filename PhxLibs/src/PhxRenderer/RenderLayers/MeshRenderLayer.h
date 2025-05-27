#pragma once

#include <PhxRenderer/RenderLayer.h>

namespace phx::gfx
{
	class MeshRenderLayer final : public RenderLayer
	{
	public:
		MeshRenderLayer() = default;
		~MeshRenderLayer() override = default;

		void* PreRender(phx::World& world, View const& view, RenderPass renderPass) override;
		void Render(RenderPass renderPass, void* cachedData) override;
		void Finalize() override;

	};
}