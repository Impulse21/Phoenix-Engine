#pragma once

#include <memory>

#include <PhxEngine/Engine/EngineCore.h>
#include <PhxEngine/RHI/RHICore.h>
#include <PhxEngine/Core/EventDispatcher.h>
#include <PhxEngine/Core/Window.h>

namespace PhxEngine
{
	class GameApplication : public IEngineApp
	{
	public:
		GameApplication() = default;

		virtual void Startup() override;
		virtual void Shutdown() override;

		virtual bool IsShuttingDown() override;
		virtual void OnTick() override;

	private:
		// TODO, put Graphics Device at this level?
		std::unique_ptr<Core::IWindow> m_window;
		RHI::SwapChain m_swapchain;

		// Add World
	};
}

