#pragma once

#include <PhxCore/EnumUtils.h>
#include <PhxRenderer/RenderSystem.h>
#include <PhxRenderer/RenderPasses.h>
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
		void CacheRenderViews(World& world);
		void CacheLayerData(World& world);

	private:
		entt::observer m_observer;
		std::vector<RenderLayer*> m_layers;

		struct PerFrameCache
		{
			uint32_t NumViews = 0;
			View* Views = nullptr;
			
			// [ view, render_pass, List of cachedData
			void** CachedData = nullptr;

		} m_perFrameCache;
	};
}

