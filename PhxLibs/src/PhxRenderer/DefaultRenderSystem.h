#pragma once

#include <PhxRenderer/RenderSystem.h>

#include <entt/entt.hpp>
namespace phx::gfx
{
	class DefaultRenderSystem final : public IRenderSystem
	{
	public:
		DefaultRenderSystem() = default;
		~DefaultRenderSystem() = default;

		void Initialize() override;
		void Finalize() override;

		void RegisterObservers(phx::World& world) override;

		void PreRender(World& world) override;
		void Render(RenderPass renderPass)  override;

		void AddLayer(RenderLayer* layer) override;

	private:
		entt::observer m_observer;
		std::vector<RenderLayer*> m_layers;
	};
}

