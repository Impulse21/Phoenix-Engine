#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/Window.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Object.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Core/HashMap.h>
#include <taskflow/taskflow.hpp>

namespace PhxEngine
{
	class Engine
	{
	public:
		static void Initialize();
		static void Finalize();
		static void Tick();

		inline static RHI::GfxDevice* GetGfxDevice() { return m_gfxDevice; }
		inline static Core::IWindow* GetWindow() { return m_window; }
		inline static tf::Executor& GetTaskExecutor() { return m_taskExecutor; }

		// Sub Systems
	private:
		inline static RHI::GfxDevice* m_gfxDevice;
		inline static Core::IWindow* m_window;
		inline static tf::Executor m_taskExecutor;

		inline static Core::unordered_map<Core::StringHash, Core::Object*> m_subsystems;
		inline static std::atomic_bool m_engineRunning = false;
	};
}