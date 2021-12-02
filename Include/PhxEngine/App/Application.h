#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <stdint.h>

// Forward Declares
struct GLFWwindow;

namespace PhxEngine
{
	class ApplicationBase
	{
	public:
		virtual ~ApplicationBase() = default;

		void Initialize(RHI::IGraphicsDevice* graphicsDevice);
		void Run();
		void Shutdown();


	public:
		void CreateGltfWindow();

		RHI::IGraphicsDevice* GetGraphicsDevice() { return this->m_graphicsDevice; }

		RHI::ICommandList* GetCommandList() { return this->m_commandList; }

	protected:
		virtual void LoadContent() {};
		virtual void Update(double elapsedTime) {};
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
		RHI::IGraphicsDevice* m_graphicsDevice;
		RHI::CommandListHandle m_commandList;

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