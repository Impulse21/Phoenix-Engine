#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Asserts.h>
#include <PhxEngine/Core/FileSystem.h>
#include <PhxEngine/App/EngineEnvironment.h>

#include <PhxEngine/Core/TimeStep.h>

#include <stdint.h>

// Forward Declares
struct GLFWwindow;

namespace PhxEngine
{
	namespace Core
	{
		class Layer;
	}

	namespace Debug
	{
		class ImGuiLayer;
	}

	namespace New
	{
		struct ApplicationCommandLineArgs
		{
			int Count = 0;
			char** Args = nullptr;

			const char* operator[](int index) const
			{
				PHX_ASSERT(index < Count);
				return Args[index];
			}
		};

		class Application
		{
		public:
			Application(
				RHI::IGraphicsDevice* GraphicsDevice,
				ApplicationCommandLineArgs args,
				std::string const& name = "Phoenix Engine");
			virtual ~Application();

			static Application* GetSingleton() { return sSingleton; }

			void Run();

			void PushBackLayer(std::shared_ptr<Core::Layer> layer);

		public:
			uint32_t GetWindowWidth() const { return this->m_windowDesc.Width; }
			uint32_t GetWindowHeight() const { return this->m_windowDesc.Height; }
			RHI::IGraphicsDevice* GetGraphicsDevice() { return this->m_graphicsDevice; }
			GLFWwindow* GetWindow() { return this->m_window; }

		protected:
			virtual void Update(Core::TimeStep const& elapsedTime);
			virtual void Render();
			
			void UpdateWindowSize();

		private:
			struct WindowDesc
			{
				uint32_t Width;
				uint32_t Height;
			};

			void CreateGltfWindow(std::string const& name, WindowDesc const& desc);

		protected:
			ApplicationCommandLineArgs m_commandLineArgs;
			WindowDesc m_windowDesc;

		private:
			const std::string m_name;
			RHI::IGraphicsDevice* m_graphicsDevice;
			GLFWwindow* m_window;

			bool m_isWindowVisible = false;
			float m_lastFrameTime = 0.0f;

			std::vector<std::shared_ptr<Core::Layer>> m_layerStack;

		private:
			static Application* sSingleton;
		};

		// To be defined in Client
		std::unique_ptr<Application> CreateApplication(ApplicationCommandLineArgs args, RHI::IGraphicsDevice* GraphicsDevice);
	}

#define AppInstance PhxEngine::New::Application::GetSingleton()

	class ApplicationBase
	{
	public:
		virtual ~ApplicationBase() = default;

		// Depericated
		void Initialize(EngineEnvironment* engineEnv);
		void RunFrame();
		void Shutdown();

	public:
		void CreateGltfWindow();

		RHI::IGraphicsDevice* GetGraphicsDevice() { return this->m_env->pGraphicsDevice; }
		Renderer::RenderSystem* GetRenderSystem() { return this->m_env->pRenderSystem; }

		RHI::ICommandList* GetCommandList() { return this->m_commandList; }
		std::shared_ptr<PhxEngine::Core::IFileSystem> GetFileSystem() { return this->m_baseFileSystem; }

		GLFWwindow* GetWindow() { return this->m_window; }

	protected:
		virtual void LoadContent() {};
		virtual void Update(double elapsedTime) {};
		virtual void Render() {};
		// Depericated
		virtual void RenderScene() {};
		virtual void RenderUI() {};

	protected:
		struct FrameStatistics
		{
			double PreviousFrameTimestamp = 0.0f;
			double AverageFrameTime = 0.0;
			double AverageTimeUpdateInterval = 0.5;
			double FrameTimeSum = 0.0;
			int NumberOfAccumulatedFrames = 0;
		};

		const FrameStatistics& GetFrameStats() const { return this->m_frameStatistics; }

	private:
		void UpdateAvarageFrameTime(double elapsedTime);
		void UpdateWindowSize();

	protected:
		static bool sGlwfIsInitialzed;
		uint32_t WindowWidth = 1280;
		uint32_t WindowHeight = 720;

	private:
		EngineEnvironment* m_env;
		RHI::CommandListHandle m_commandList;

		std::shared_ptr<PhxEngine::Core::IFileSystem> m_baseFileSystem;

		GLFWwindow* m_window;

		FrameStatistics m_frameStatistics = {};
		bool m_isWindowVisible = false;
	};
}

#define CREATE_APPLICATION_FUNCTION PhxEngine::ApplicationBase* PhxEngine::CreateApplication

#define CREATE_APPLICATION( app_Class )															\
	CREATE_APPLICATION_FUNCTION()																\
	{																							\
		return new app_Class();																	\
	}																							\