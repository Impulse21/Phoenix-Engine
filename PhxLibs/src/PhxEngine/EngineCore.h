#pragma once


namespace phx
{
	class IApplication;
	extern IApplication* CreateApplication();

	namespace EngineCore
	{
		extern size_t g_FrameCount;
		void PreInitialize(int argc, wchar_t** argv);

		void Initialize(void* windowHandle);
		void Tick();

		void Finalize();
	};
}

