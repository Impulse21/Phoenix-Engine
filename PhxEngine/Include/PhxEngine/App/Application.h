#pragma once

#include <atomic>
#include "PhxEngine/Core/StopWatch.h"

#include "PhxEngine/Core/Canvas.h"
#include "PhxEngine/Core/Platform.h"
#include "PhxEngine/Core/FrameProfiler.h"
#include "PhxEngine/Graphics/RHI/PhxRHI.h"


// -- New
#include "PhxEngine/App/Layer.h"
#include "PhxEngine/App/LayerStack.h"
#include "PhxEngine/Core/Span.h"
#include "PhxEngine/Core/Event.h"

#define NUM_BACK_BUFFERS 3

namespace PhxEngine
{
	namespace Core
	{
		class IWindow;
	}
	namespace Graphics
	{
		class ShaderStore;
		class ImGuiRenderer;
	}

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
		uint32_t WindowWidth = 2000;
		uint32_t WindowHeight = 1200;
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
		// This Prevets us from having more then one application. This is an okay limitation for now as I don't have a use case.
		inline static LayeredApplication* Ptr = nullptr;

	public:
		LayeredApplication(ApplicationSpecification const& spec);
		virtual ~LayeredApplication();
		
		void Run();

		virtual void OnInit() {};

		void PushLayer(std::shared_ptr<AppLayer> layer);
		void PushOverlay(std::shared_ptr<AppLayer> layer);

		virtual void OnEvent(Core::Event& e);

		Core::IWindow* GetWindow() { return this->m_window.get(); }

		void Close() { this->m_isRunning = false; }

		uint64_t GetFrameCount() const { return this->m_frameCount; }

		const ApplicationSpecification& GetSpec() { return this->m_spec; }

	private:
		void RenderImGui();

		// TODO: have a think about how this will Work
		void Render();
		void Compose();

	private:
		PhxEngine::RHI::CommandListHandle m_composeCommandList;

		const ApplicationSpecification m_spec;
		uint64_t m_frameCount = 0;

		std::unique_ptr<Core::IWindow> m_window;
		std::unique_ptr<Graphics::ShaderStore> m_shaderStore;
		std::shared_ptr<Graphics::ImGuiRenderer> m_imguiRenderer;

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

	public:
		PhxEngine::RHI::IGraphicsDevice* GetGraphicsDevice() { return this->m_graphicsDevice; }
		const Core::Canvas& GetCanvas() const { return this->m_canvas; }
		uint64_t GetFrameCount() const { return this->m_frameCount; }

	private:
		bool m_isInitialized;
		std::atomic_bool m_initializationComplete;

		uint64_t m_frameCount = 0;

		Core::StopWatch m_stopWatch;

		PhxEngine::RHI::IGraphicsDevice* m_graphicsDevice = nullptr;
		Core::Platform::WindowHandle m_windowHandle = nullptr;
		Core::Canvas m_canvas;

		// RHI Resources
		PhxEngine::RHI::CommandListHandle m_composeCommandList;
		PhxEngine::RHI::CommandListHandle m_beginFrameCommandList;

		std::unique_ptr<Core::FrameProfiler> m_frameProfiler;

	};

	// Defined by client
	LayeredApplication* CreateApplication(int argc, char** argv);
}

