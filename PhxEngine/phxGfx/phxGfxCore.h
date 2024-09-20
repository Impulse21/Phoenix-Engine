#pragma once

#include "phxGfxResources.h"

namespace phx::gfx
{
	constexpr size_t kBufferCount = 3;

	void InitializeNull();

#if defined(PHX_PLATFORM_WINDOWS)
	void InitializeWindows(SwapChainDesc const& desc, HWND hWnd);
#endif

	void Finalize();

	class Device
	{
	public:
		inline static Device* Ptr = nullptr;

		virtual void WaitForIdle() = 0;
		virtual void ResizeSwapChain(SwapChainDesc const& desc) = 0;
		virtual void Present() = 0;

	public:
		virtual ~Device() = default;
	};

	class GpuMemoryManager
	{
	public:
		inline static GpuMemoryManager* Ptr = nullptr;

	public:
		virtual ~GpuMemoryManager() = default;
	};

	class ResourceManager
	{
	public:
		inline static ResourceManager* Ptr = nullptr;

		virtual void RunGrabageCollection(uint64_t completedFrame = ~0u) = 0;

	public:
		virtual ~ResourceManager() = default;
	};

	class CommandList;
	class Renderer
	{
	public:
		inline static Renderer* Ptr = nullptr;

		virtual CommandList* BeginCommandRecording(CommandQueueType type = CommandQueueType::Graphics) = 0;
		virtual void SubmitCommands() = 0;

	public:
		virtual ~Renderer() = default;
	};

	class RenderPassRenderer;
	class CommandList
	{
	public:
		virtual ~CommandList() = default;
		virtual RenderPassRenderer* BeginRenderPass() = 0;
		virtual void EndRenderPass(RenderPassRenderer* pass) = 0;
	};

	class RenderPassRenderer
	{
	public:
		virtual ~RenderPassRenderer() = default;
	};
}