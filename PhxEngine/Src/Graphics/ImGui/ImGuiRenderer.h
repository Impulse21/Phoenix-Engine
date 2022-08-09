#pragma once

#include <memory>

#include "PhxEngine/App/Layer.h"
#include "PhxEngine/Core/Platform.h"
#include "PhxEngine/Graphics/RHI/PhxRHI.h"

// Forward Declares
struct ImGuiContext;

namespace PhxEngine::Graphics
{
	class ShaderStore;

	class ImGuiRenderer
	{
	public:
		ImGuiRenderer() = default;
		virtual ~ImGuiRenderer() = default;

		void Initialize(
			RHI::IGraphicsDevice* graphicsDevice,
			ShaderStore const& shaderStore,
			Core::Platform::WindowHandle windowHandle);
		void Finalize();

		void BeginFrame();
		void Render(RHI::CommandListHandle cmd);

	private:
		void CreatePipelineStateObject(
			RHI::IGraphicsDevice* graphicsDevice,
			ShaderStore const& shaderStore);

	private:
		ImGuiContext* m_imguiContext;

		RHI::TextureHandle m_fontTexture;
		RHI::GraphicsPSOHandle m_pso;
	};
}