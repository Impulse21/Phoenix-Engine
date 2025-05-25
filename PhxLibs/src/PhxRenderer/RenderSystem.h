#pragma once

#include <memory>
#include <PhxRenderer/RenderPasses.h>
#include <PhxRenderer/RenderLayer.h>
#include <DirectXMath.h>

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

		DirectX::XMFLOAT4X4 View;
		DirectX::XMFLOAT4X4 Projection;
		DirectX::XMFLOAT4X4 ViewProjection;

		DirectX::XMFLOAT4X4 ViewInv;
		DirectX::XMFLOAT4X4 ProjectionInv;
		DirectX::XMFLOAT4X4 ViewProjectionInv;

		Core::Frustum FrustumWS;
		Core::Frustum FrustumVS;
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