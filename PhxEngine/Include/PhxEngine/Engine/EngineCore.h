#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/Window.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Object.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Renderer/AsyncGpuUploader.h>

#include <TaskScheduler.h>

namespace PhxEngine
{
	class IEngineApp
	{
	public:
		virtual ~IEngineApp() = default;

		virtual void Initialize() = 0;
		virtual void InitializeAsync() = 0;
		virtual void Finalize() = 0;

		virtual bool IsShuttingDown() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;
		virtual void OnCompose(RHI::CommandListHandle composeCmdList) = 0;

	};

	void Run(IEngineApp& engingApp);

	RHI::GfxDevice* GetGfxDevice();
	Core::IWindow* GetWindow();
	enki::TaskScheduler& GetTaskScheduler();
	PhxEngine::Renderer::AsyncGpuUploader& GetAsyncLoader();

}

#ifdef _MSC_VER // Windows
#include <Windows.h>
#define MainFunc int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else // Linux
#define MainFunc int main(int argc, char** argv)
#endif

#define PHX_CREATE_APPLICATION(class_name)		\
    MainFunc									\
    {											\
        class_name app;							\
        PhxEngine::Run(app);					\
		return 0;								\
    }
