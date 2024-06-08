#pragma once

#include "phxApplication.h"

namespace phx::editor
{
	class Editor final : public phx::IApplication
	{
	public:
		Editor() = default;

		void OnStartup() override;
		void OnShutdown() override;

		void OnPreRender(Subflow* subflow) override;
		void OnRender(Subflow* subflow) override;
		void OnUpdate(Subflow* subflow) override;

		void OnSuspend() override { this->m_isSuspended = true; };
		void OnResume() override { this->m_isSuspended = false; };

		bool EnableMultiThreading() override { return true; }

		void GetDefaultWindowSize(uint32_t& width, uint32_t& height) override { width = 2000; height = 1200; };

	private:
		bool m_isSuspended = false;
	};
}

