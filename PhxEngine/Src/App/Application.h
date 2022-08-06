#pragma once

#include <atomic>
#include "Core/StopWatch.h"

#include "Core/Canvas.h"
#include "Core/Platform.h"
#include "Core/FrameProfiler.h"
#include "Graphics/RHI/PhxRHI.h"
#include "Graphics/ShaderStore.h"
#include "Graphics/ShaderFactory.h"
#include "Graphics/ImGui/ImGuiRenderer.h"
#include "Graphics/RenderPathComponent.h"


// -- New
#include "App/Layer.h"
#include "App/LayerStack.h"
#include "Core/Span.h"

#define NUM_BACK_BUFFERS 3

namespace PhxEngine
{
	struct CommandLineArgs
	{

	};

	struct RendererConfig
	{
		RHI::GraphicsAPI GraphicsAPI = RHI::GraphicsAPI::Unknown;
		uint32_t FramesInFlight = 3;
	};
	struct ApplicationSpecification
	{
		std::string Name = "";
		uint32_t WindowHeight = 1600;
		uint32_t WindowWidth = 900;
		bool FullScreen = false;
		bool VSync = true;
		bool EnableImGui = false;
		bool AllowWindowResize = true;

		std::string WorkingDirectory = "";

		RendererConfig RendererConfig;
	};

	class LayeredApplication
	{
	public:
		LayeredApplication(ApplicationSpecification const& spec);
		virtual ~LayeredApplication();
		
		void Run();

		virtual void OnInit() {};

		void PushLayer(AppLayer* layer);
		void PushOverlay(AppLayer* layer);

	private:
		void RenderImGui();

	private:
		const ApplicationSpecification m_spec;

		LayerStack m_layerStack;
		bool m_isRunning = true;
		bool m_isMinimized = false;
	};

	class Application
	{
	public:
		virtual void Initialize(PhxEngine::RHI::IGraphicsDevice* graphicsDevice);
		virtual void Finalize();

		void Run();

		void Tick();

		//void FixedUpdate();
		void Update(Core::TimeStep deltaTime);
		void Render();
		void Compose(PhxEngine::RHI::CommandListHandle cmdList);

		void SetWindow(Core::Platform::WindowHandle windowHandle, bool isFullscreen = false);
		void AttachRenderPath(std::shared_ptr<Graphics::RenderPathComponent> renderPathComponent);

	public:
		PhxEngine::RHI::IGraphicsDevice* GetGraphicsDevice() { return this->m_graphicsDevice; }
		const Core::Canvas& GetCanvas() const { return this->m_canvas; }
		Graphics::ShaderStore& GetShaderStore() { return this->m_shaderStore; }
		uint64_t GetFrameCount() const { return this->m_frameCount; }

	private:
		bool m_isInitialized;
		std::atomic_bool m_initializationComplete;

		uint64_t m_frameCount = 0;

		Core::StopWatch m_stopWatch;

		Graphics::ShaderStore m_shaderStore;
		PhxEngine::RHI::IGraphicsDevice* m_graphicsDevice = nullptr;
		Core::Platform::WindowHandle m_windowHandle = nullptr;
		Core::Canvas m_canvas;

		// RHI Resources
		PhxEngine::RHI::CommandListHandle m_composeCommandList;
		PhxEngine::RHI::CommandListHandle m_beginFrameCommandList;

		PhxEngine::Graphics::ImGuiRenderer m_imguiRenderer;
		std::unique_ptr<Core::FrameProfiler> m_frameProfiler;

		std::shared_ptr<Graphics::RenderPathComponent> m_renderPath;
	};

	// Defined by client
	LayeredApplication* CreateApplication(int argc, char** argv);
}

