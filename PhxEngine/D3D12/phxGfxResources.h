#pragma once

#include "pch.h"
#include "phxGfxCommonResources.h"
#include "phxDeferredReleaseQueue.h"

namespace phx::gfx
{
	struct SwapChain : NonCopyable
	{
        SwapChainDesc m_Desc;
        DeferredReleasePtr<IDXGISwapChain1> m_platform;
        DeferredReleasePtr<IDXGISwapChain4> m_platform4;

		IDXGISwapChain1* GetPlatform() { return this->m_platform; }
		IDXGISwapChain2* GetPlatform2() { return this->m_platform2; }

		const SwapChainDesc& GetDesc() { return this->m_Desc; }
	};
}