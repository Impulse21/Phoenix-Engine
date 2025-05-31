#pragma once


namespace phx
{
	class IApplication;
	extern IApplication* CreateApplication();
	extern void DeleteApplication(IApplication* ptr);

	namespace EngineCore
	{
		void PreInitialize(int argc, wchar_t** argv);

		void Initialize(void* windowHandle);
		void Tick();

		void Finalize();
	};
}

