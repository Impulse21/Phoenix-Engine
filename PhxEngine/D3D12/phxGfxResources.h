#pragma once

#include "pch.h"
#include "phxGfxCommonResources.h"
#include "phxDeferredReleaseQueue.h"
#include <vector>

namespace phx::gfx
{
	struct D3D12SwapChain : NonCopyable
	{
		Microsoft::WRL::ComPtr<IDXGISwapChain1> m_platform;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> m_platform4;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_backBuffers;

		IDXGISwapChain1* GetPlatform() { return this->m_platform.Get(); }
		IDXGISwapChain1* GetPlatform4() { return this->m_platform4.Get(); }
	};

	using PlatformSwapChain = D3D12SwapChain;
}