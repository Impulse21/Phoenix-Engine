#pragma once

#include <atomic>
#include "PhxEngine/Core/StopWatch.h"

#include "PhxEngine/Core/Canvas.h"
#include "PhxEngine/Core/Platform.h"
#include "PhxEngine/Core/FrameProfiler.h"
#include "PhxEngine/RHI/PhxRHI.h"


#include "PhxEngine/Engine/Layer.h"
#include "PhxEngine/Engine/LayerStack.h"
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

	namespace Renderer
	{
		class ShaderCodeLibrary;
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
		bool VSync = false;
		bool EnableImGui = false;
		bool AllowWindowResize = true;

		std::string WorkingDirectory = "";

		RendererConfig RendererConfig;
	};

	class EngineApp
	{
	public:
		// This Prevets us from having more then one application. This is an okay limitation for now as I don't have a use case.
		inline static EngineApp* GPtr = nullptr;

	public:
		EngineApp(ApplicationSpecification const& spec);
		virtual ~EngineApp();
		
		void PreInitialize();
		void Initialize();
		void Finalize();

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
		const ApplicationSpecification m_spec;
		uint64_t m_frameCount = 0;

		// Application state
		LayerStack m_layerStack;
		bool m_isRunning = true;
		bool m_isMinimized = false;
		Core::StopWatch m_stopWatch;

		// Interna systems
		std::unique_ptr<Core::IWindow> m_window;
		std::unique_ptr<Graphics::ShaderStore> m_shaderStore;
		std::shared_ptr<Graphics::ImGuiRenderer> m_imguiRenderer;

		// -- Render Stuff ---
		RHI::RHIViewportHandle m_viewport;
	};

	// Defined by client
	EngineApp* CreateApplication(int argc, char** argv);
}

