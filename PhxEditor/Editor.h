#pragma once

#include "phxApplication.h"

namespace phx::editor
{
	class Editor : public phx::IApplication
	{
	public:
		Editor() = default;

		void OnStartup() override;
		void OnShutdown() override;

		void OnPreRender() override;
		void OnRender() override;
		void OnUpdate() override;

		bool EnableMultiThreading() override { return true; }
	};
}

