#pragma once

namespace PhxEngine
{
	class IEngineApp
	{
	public:
		virtual ~IEngineApp() = default;

		virtual void Startup() = 0;
		virtual void Shutdown() = 0;

		virtual bool IsShuttingDown() = 0;
		virtual void OnTick() = 0;

	};

	namespace EngineCore
	{
		void Initialize();
		void RunApplication(IEngineApp& engingApp);
		void Finalize();
	}
}
