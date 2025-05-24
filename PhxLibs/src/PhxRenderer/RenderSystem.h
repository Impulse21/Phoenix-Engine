#pragma once

#include <memory>
#include <PhxRenderer/RenderPasses.h>
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
	struct View
	{

	};

	template<typename T>
	concept RenderLayerType = std::is_base_of_v<RenderLayer, T>;

	struct View;

	class IRenderSystem
	{
	public:
		inline static IRenderSystem* Ptr = nullptr;

	public:
		virtual ~IRenderSystem() = default;

		virtual void Initialize() = 0;
		virtual void Finalize() = 0;

		virtual void RegisterObservers(phx::World& world) = 0;

		virtual void PreRender(World& world) = 0;
		virtual void Render(RenderPass renderPass) = 0;

		virtual void AddLayer(RenderLayer* layer) = 0;

		template<RenderLayerType TLayer>
		void AddLayer()
		{
			AddLayer(new TLayer);
		}
	};
}