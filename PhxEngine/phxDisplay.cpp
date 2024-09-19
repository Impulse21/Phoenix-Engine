#include "pch.h"

#include <cmath>

#include "phxDisplay.h"
#include "phxGfxCore.h"
#include "phxGfxCommonResources.h"
#include "phxDeferredReleaseQueue.h"
#include "phxSystemTime.h"

namespace phx::EngineCore { extern HWND g_hWnd; }

using namespace phx;

#define SWAP_CHAIN_BUFFER_COUNT 2

namespace
{
	constexpr gfx::Format kSwapChainFromat = gfx::Format::R10G10B10A2_UNORM;
	float m_frameTime = 0.0f;
	uint64_t m_frameIndex = 0;
	int64_t m_frameStartTick = 0;

	bool m_enableVSync = false;

	gfx::SwapChain m_swapChain;
}

namespace phx::gfx
{
	uint32_t g_NativeWidth = 0;
	uint32_t g_NativeHeight = 0;
	uint32_t g_DisplayWidth = 1920;
	uint32_t g_DisplayHeight = 1080;
	bool g_EnableHDROutput = false;

	uint64_t GetFrameCount()
	{
		return m_frameIndex;
	}
	float GetFrameTime()
	{
		return m_frameTime;
	}
	float GetFrameRate()
	{
		return m_frameTime == 0.0f ? 0.0f : 1.0f / m_frameTime;
	}
}

namespace phx::Display
{
	void Initialize()
	{
		gfx::SwapChainDesc desc = {
			.Width = gfx::g_DisplayWidth,
			.Height = gfx::g_DisplayHeight,
			.BufferCount = SWAP_CHAIN_BUFFER_COUNT,
			.Format = kSwapChainFromat,
			.Fullscreen = false,
			.VSync = false,
			.EnableHDR = false
		};

		gfx::ResourceManager::CreateSwapChain(desc, m_swapChain);
	}

	void Finalize()
	{
		gfx::IdleGpu();
		gfx::ResourceManager::Release(m_swapChain);
	}

	void Resize(uint32_t width, uint32_t height)
	{
		gfx::IdleGpu();
		gfx::g_DisplayWidth = width;
		gfx::g_DisplayHeight = height;

		gfx::SwapChainDesc desc = {
			.Width = gfx::g_DisplayWidth,
			.Height = gfx::g_DisplayHeight,
			.BufferCount = SWAP_CHAIN_BUFFER_COUNT,
			.Format = kSwapChainFromat,
			.Fullscreen = false,
			.VSync = false,
			.EnableHDR = false
		};

		gfx::ResourceManager::CreateSwapChain(desc, m_swapChain);
	}

	void Preset()
	{
		UINT presentInterval = m_enableVSync ? std::min(4, (int)std::round(m_frameTime * 60.0f)) : 0;
		gfx::SubmitFrame(m_swapChain);

		int64_t currentTick = SystemTime::GetCurrentTick();

#if false

		if (s_EnableVSync)
		{
			// With VSync enabled, the time step between frames becomes a multiple of 16.666 ms.  We need
			// to add logic to vary between 1 and 2 (or 3 fields).  This delta time also determines how
			// long the previous frame should be displayed (i.e. the present interval.)
			s_FrameTime = (s_LimitTo30Hz ? 2.0f : 1.0f) / 60.0f;
			if (s_DropRandomFrames)
			{
				if (std::rand() % 50 == 0)
					s_FrameTime += (1.0f / 60.0f);
			}
		}
		else
		{
			// When running free, keep the most recent total frame time as the time step for
			// the next frame simulation.  This is not super-accurate, but assuming a frame
			// time varies smoothly, it should be close enough.
			s_FrameTime = (float)SystemTime::TimeBetweenTicks(s_FrameStartTick, currentTick);
		}
#endif

		m_frameStartTick = currentTick;

		++m_frameIndex;

	}
}