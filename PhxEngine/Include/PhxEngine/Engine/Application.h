#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/Window.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Object.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Core/HashMap.h>

namespace PhxEngine
{
	class IEngineApp
	{
	public:
		virtual ~IEngineApp() = default;

		virtual void PreInitialize() {};
		virtual void Initialize() = 0;
		virtual void Finalize() = 0;

		virtual bool IsShuttingDown() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;
		virtual void OnCompose(RHI::CommandListHandle composeCmdList) = 0;

	};

	void Run(IEngineApp& app);
}
