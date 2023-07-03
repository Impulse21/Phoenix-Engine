#pragma once
#include <memory>
#include <string>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Engine/ApplicationEvents.h>
#include <PhxEngine/Core/Profiler.h>
#include <DirectXMath.h>
#include <PhxEngine/Core/Memory.h>


namespace PhxEngine
{
	namespace Core
	{
		class IWindow;
	}

	struct EngineParam
	{
		RHI::GraphicsAPI GraphicsAPI = RHI::GraphicsAPI::Unknown;
		uint32_t FramesInFlight = 3;

		std::string Name = "";
		uint32_t WindowWidth = 1280;
		uint32_t WindowHeight = 720;
		bool FullScreen = false;
		bool VSync = false;
		bool EnableImGui = false;
		bool AllowWindowResize = true;

		size_t MaximumDynamicSize = PhxMB(32);
	};

	class IEngineApplication
	{
	public:
		virtual bool Initialize() = 0;
		virtual bool Finalize() = 0;
		virtual bool RunFrame() = 0;
		virtual bool ShuttingDown() const = 0;

		virtual ~IEngineApplication() = default;
	};

	bool EngineInitialize(EngineParam const& params);
	bool EngineRun(IEngineApplication& app);
	bool EngineFinalize();
}