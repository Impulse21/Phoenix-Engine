#pragma once

namespace PhxEngine
{
	class IEngineApp
	{
	public:
		virtual ~IEngineApp() = default;

		virtual void Initialize() = 0;
		virtual void Finalize() = 0;

		virtual bool IsShuttingDown() = 0;
		virtual void OnTick() = 0;

	};

	namespace EngineLoop
	{
		void Run(IEngineApp& engingApp);
	}
}

#ifdef _MSC_VER // Windows
#include <Windows.h>
#define MainFunc int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else // Linux
#define MainFunc int main(int argc, char** argv)
#endif

#define PHX_DECLARE_MAIN(class_name)        \
    MainFunc								\
    {										\
        class_name app;						\
        EngineLoop::Run(app);    \
		return 0;							\
    }
