#include "pch.h"

#include <cmath>

#include "phxDisplay.h"
#include "EmberGfx/phxEmber.h"
#if false
#include "phxGfxCore.h"
#include "phxDeferredReleaseQueue.h"
#include "phxSystemTime.h"
#endif

namespace phx::EngineCore { extern HWND g_hWnd; }

using namespace phx;

#define SWAP_CHAIN_BUFFER_COUNT 2

namespace
{
	float m_frameTime = 0.0f;
	uint64_t m_frameIndex = 0;
	int64_t m_frameStartTick = 0;

	bool m_enableVSync = false;
}

namespace phx::gfx
{
	uint32_t g_NativeWidth = 0;
	uint32_t g_NativeHeight = 0;
	uint32_t g_DisplayWidth = 1920;
	uint32_t g_DisplayHeight = 1080; 
	gfx::Format g_SwapChainFormat = gfx::Format::R10G10B10A2_UNORM;
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
		.Format = gfx::g_SwapChainFormat,
		.Fullscreen = false,
		.VSync = false,
		.EnableHDR = false
		};

		phx::gfx::InitializeWindows(
			desc,
			EngineCore::g_hWnd);
	}

	void Finalize()
	{
		phx::gfx::Finalize();
	}

	void Resize(uint32_t width, uint32_t height)
	{
		gfx::GfxDevice& device = gfx::Ember::Ptr->GetDevice();

		gfx::g_DisplayWidth = width;
		gfx::g_DisplayHeight = height;

		gfx::SwapChainDesc desc = {
			.Width = gfx::g_DisplayWidth,
			.Height = gfx::g_DisplayHeight,
			.Format = gfx::g_SwapChainFormat,
			.Fullscreen = false,
			.VSync = false,
			.EnableHDR = false
		};

		device.ResizeSwapChain(desc);
	}

	void Preset()
	{
		phx::gfx::GfxDevice& gfxDevice = phx::gfx::Ember::Ptr->GetDevice();
		gfxDevice.SubmitFrame();
#if false
		UINT presentInterval = m_enableVSync ? std::min(4, (int)std::round(m_frameTime * 60.0f)) : 0;
		int64_t currentTick = SystemTime::GetCurrentTick();

		gfx::Renderer::Ptr->SubmitCommands();
		gfx::Device::Ptr->Present();

		m_frameStartTick = currentTick;
		++m_frameIndex;

		// gfx::ResourceManager::Ptr->RunGrabageCollection(m_frameIndex);

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

#endif

	}
}