#pragma once

#include "Core/phxPlatform.h"

namespace phx
{
	class IApplication;
	namespace Engine
	{
		struct EngineParams
		{
			core::WindowHandle Window;
			int WindowWidth;
			int WindowHeight;

		};

		void Initialize(IApplication& app, EngineParams const& desc);
		void Tick(IApplication& app);
		void Finialize(IApplication& app);
	};
}