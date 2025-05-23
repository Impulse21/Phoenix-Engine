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

		void OnPreRender(World& world) override;
		void OnRender()  override;

		void RegisterSubSystem(uint32_t passMask, std::shared_ptr<IRenderSubSystem> subSystem) override;

	private:
		entt::observer m_observer;
		std::vector<std::shared_ptr<IRenderSubSystem>> m_subsystems;

		struct CachedResource
		{
			RefCountPtr<IResource> Resource;
		};

		size_t m_cachedResourceCount;
		CachedResource* m_cachedResourceData;
	};
}

