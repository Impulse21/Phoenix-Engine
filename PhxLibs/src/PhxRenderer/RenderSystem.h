#pragma once

#include <memory>
#include <PhxRenderer/RenderLayer.h>

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
	template<typename T>
	concept RenderLayerType = std::is_base_of_v<RenderLayer, T>;

	class IRenderSystem
	{
	public:
		inline static IRenderSystem* Ptr = nullptr;

	public:
		virtual ~IRenderSystem() = default;

		virtual void Initialize() = 0;
		virtual void Finalize() = 0;

		virtual void RegisterObservers(phx::World& world) = 0;

		virtual void OnPreRender(World& world) = 0;
		virtual void OnRender() = 0;

		virtual void RegisterSubSystem(RenderLayer* layer) = 0;

		template<RenderLayerType TLayer>
		void AddLayer()
		{
			RegisterSubSystem(TLayer);
		}
	};
}