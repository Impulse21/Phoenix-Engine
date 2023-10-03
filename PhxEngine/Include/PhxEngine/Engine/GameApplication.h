#pragma once

#include <memory>

#include <PhxEngine/Engine/EngineLoop.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/EventDispatcher.h>
#include <PhxEngine/Core/Window.h>

#include <PhxEngine/Renderer/ImGuiRenderer.h>

namespace PhxEngine
{
	class GameApplication : public IEngineApp
	{
	public:
		GameApplication() = default;

		virtual void Initialize() override;
		virtual void Finalize() override;

		virtual bool IsShuttingDown() override;
		virtual void OnTick() override;

		virtual void Startup();
		virtual void Shutdown();
		virtual void OnRender() {};

	private:
		std::unique_ptr<Core::IWindow> m_window;
		// Add World
	};
}
