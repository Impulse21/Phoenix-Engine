#pragma once

#include <PhxCore/Base.h>
#include <PhxRenderer/RenderSystem.h>
#include <PhxResource/IResource.h>

#include <vector>
#include <memory>

#include <entt/entt.hpp>

namespace phx
{
	class World;
}

namespace phx::gfx
{

	struct RenderMeshComponent
	{
		RefCountPtr<IResource> MeshResource;
	};
	namespace ForwardRenderPasses
	{
		enum
		{
			GeometryPass	= BIT(0),
			ShadowPass		= BIT(1),
			GuiPass			= BIT(2),
		};
	}

	class ForwardRenderer final
	{
	public:
		ForwardRenderer() = default;
		~ForwardRenderer() = default;

		void RegisterObserver(phx::World& world);

		void Finalize();

		template<typename TSystem>
		void RegisterRenderSystem(uint32_t passMask)
		{
			RegisterRenderSystem(passMask, std::make_shared<TSystem>());
		}

		template<typename TSystem>
		void RegisterRenderSystem(uint32_t passMask, std::shared_ptr<TSystem> renderSystem)
		{
			m_renderSystems.emplace_back(std::move(renderSystem));
			m_passMasks.push_back(passMask);
			m_cachedData.push_back(nullptr);
		}

		void OnPreRender(World& world);
		void OnRender();

	private:
		std::vector<std::shared_ptr<IRenderSubSystem>> m_renderSystems;
		std::vector<uint32_t> m_passMasks;
		std::vector<void*> m_cachedData;

		entt::observer m_observer;
	};

}

