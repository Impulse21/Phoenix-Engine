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

		void OnPreRender(Subflow* subflow) override;
		void OnRender(Subflow* subflow) override;
		void OnUpdate(Subflow* subflow) override;

		bool EnableMultiThreading() override { return true; }
	};
}

