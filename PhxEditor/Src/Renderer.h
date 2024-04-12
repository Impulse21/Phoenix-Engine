#pragma once

#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Core/EnumArray.h>

#include <PhxEngine/Core/Pool.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <filesystem>

namespace PhxEditor
{
	struct ViewportImpl
	{
		PhxEngine::RHI::TextureHandle ColourBuffer;
	};

	enum class ShaderTypes
	{
		VS_Triangle,
		PS_Triangle,

		VS_FullScreenQuad,
		PS_FullScreenQuad,

		Count,
	};

	class Renderer final : public PhxEngine::IRenderer
	{
	public:
		Renderer();
		~Renderer() override = default;

		PhxEngine::ViewportHandle ViewportCreate() override;
		void ViewportResize(uint32_t width, uint32_t height, PhxEngine::ViewportHandle viewport) override;
		DirectX::XMFLOAT2 ViewportGetSize(PhxEngine::ViewportHandle viewport) override;
		PhxEngine::RHI::TextureHandle ViewportGetColourBuffer(PhxEngine::ViewportHandle handle) override;

		void OnUpdate() override;
		// Register Systems for Updating

		void LoadShadersAsync();
		void LoadShaders();

		PhxEngine::Span<PhxEngine::RHI::ShaderHandle> GetShaderList() const override;

	private:
		PhxEngine::EnumArray<ShaderTypes, PhxEngine::RHI::ShaderHandle> m_loadedShaders;

	};
}

