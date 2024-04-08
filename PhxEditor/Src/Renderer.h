#pragma once

#include <PhxEngine/Renderer/Renderer.h>

#include <PhxEngine/Core/Pool.h>

namespace PhxEditor
{
	struct ViewportImpl
	{
		PhxEngine::RHI::TextureHandle ColourBuffer;
	};

	class Renderer final : public PhxEngine::IRenderer
	{
	public:
		Renderer() = default;
		~Renderer() override = default;

		PhxEngine::ViewportHandle ViewportCreate() override;
		void ViewportResize(uint32_t width, uint32_t height, PhxEngine::ViewportHandle viewport) override;
		DirectX::XMFLOAT2 ViewportGetSize(PhxEngine::ViewportHandle viewport) override;
		PhxEngine::RHI::TextureHandle ViewportGetColourBuffer(PhxEngine::ViewportHandle handle) override;

		void OnUpdate() override;
		// Register Systems for Updating

	private:

	};
}

