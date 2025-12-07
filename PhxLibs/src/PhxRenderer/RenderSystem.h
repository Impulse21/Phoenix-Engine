#pragma once

#include <memory>
#include <PhxRenderer/RenderPasses.h>
#include <PhxRenderer/RenderLayer.h>
#include <hlsl++.h>

namespace phx
{
	class World;
}

namespace phx::gfx
{
	struct View
	{

		hlslpp::float4x4 ViewMatrix;
		hlslpp::float4x4 ProjectionMatrix;
		hlslpp::float4x4 WorldToClipMatrix; // View - project matrix

		hlslpp::float4x4 InvViewMatrix;
		hlslpp::float4x4 InvProjectionMatrix;
		hlslpp::float4x4 InvWorldToClipMatrix;
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