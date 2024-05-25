#pragma once

#include "Core/phxPlatform.h"

namespace phx
{
	class IApplication;
	namespace EngineCore
	{
		struct EngineParams
		{
			core::WindowHandle Window;
			int WindowWidth;
			int WindowHeight;

		};

		void InitializeApplication(IApplication& app, EngineParams const& desc);
		void Tick(IApplication& app);
		void FinializeApplcation(IApplication& app);
	};
}