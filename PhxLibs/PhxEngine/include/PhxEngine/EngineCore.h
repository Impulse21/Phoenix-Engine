#pragma once

namespace phx
{
	class IApplication;

	extern IApplication* CreateApplication();

	namespace EngineCore
	{
		void Run(int argc, char* argv[]);
		void RequestExit();
	}
}

