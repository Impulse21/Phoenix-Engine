#pragma once

#include <atomic>
#include "StopWatch.h"

#include "Core/Canvas.h"
#include "Core/Platform.h"
#include "Core/FrameProfiler.h"
#include "Graphics/RHI/PhxRHI.h"
#include "Graphics/ShaderStore.h"
#include "Graphics/ShaderFactory.h"
#include "Graphics/ImGui/ImGuiRenderer.h"
#include "Graphics/RenderPathComponent.h"

#define NUM_BACK_BUFFERS 3

namespace PhxEngine::Core
{
	struct CommandLineArgs
	{

	};

	class Application
	{
	public:
		virtual void Initialize(PhxEngine::RHI::IGraphicsDevice* graphicsDevice);
		virtual void Finalize();

		void Tick();

		//void FixedUpdate();
		void Update(TimeStep deltaTime);
		void Render();
		void Compose(PhxEngine::RHI::CommandListHandle cmdList);

		void SetWindow(Core::Platform::WindowHandle windowHandle, bool isFullscreen = false);
		void AttachRenderPath(std::shared_ptr<Graphics::RenderPathComponent> renderPathComponent);

	public:
		PhxEngine::RHI::IGraphicsDevice* GetGraphicsDevice() { return this->m_graphicsDevice; }
		const Canvas& GetCanvas() const { return this->m_canvas; }
		Graphics::ShaderStore& GetShaderStore() { return this->m_shaderStore; }
		uint64_t GetFrameCount() const { return this->m_frameCount; }
	private:
		bool m_isInitialized;
		std::atomic_bool m_initializationComplete;

		uint64_t m_frameCount = 0;

		StopWatch m_stopWatch;

		Graphics::ShaderStore m_shaderStore;
		PhxEngine::RHI::IGraphicsDevice* m_graphicsDevice = nullptr;
		Core::Platform::WindowHandle m_windowHandle = nullptr;
		Core::Canvas m_canvas;

		// RHI Resources
		PhxEngine::RHI::CommandListHandle m_composeCommandList;
		PhxEngine::RHI::CommandListHandle m_beginFrameCommandList;

		PhxEngine::Graphics::ImGuiRenderer m_imguiRenderer;
		std::unique_ptr<FrameProfiler> m_frameProfiler;

		std::shared_ptr<Graphics::RenderPathComponent> m_renderPath;
	};

	// Defined by client
	Application* CreateApplication(CommandLineArgs args);
}

