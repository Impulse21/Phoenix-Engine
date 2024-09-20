#pragma once

#include "phxGfxResources.h"

namespace phx::gfx
{
	constexpr size_t kBufferCount = 3;

	void InitializeNull();

#if defined(PHX_PLATFORM_WINDOWS)
	void InitializeWindows(HWND hWnd);
#endif

	void Shutdown();

	class Device
	{
	public:
		inline static Device* Ptr = nullptr;

	public:
		virtual ~Device() = default;
	};

	class ResourceManager
	{
	public:
		inline static ResourceManager* Ptr = nullptr;

	public:
		virtual ~ResourceManager() = default;
	};

	class Renderer
	{
	public:
		inline static Renderer* Ptr = nullptr;
	public:
		virtual ~Renderer() = default;
	};
}