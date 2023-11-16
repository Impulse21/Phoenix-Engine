#pragma once

#include <RHI/D3D12/D3D12GfxDevice.h>
#include <RHI/D3D12/D3D12CommandList.h>
#include <RHI/D3D12/D3D12Resources.h>

namespace PhxEngine::RHI
{
	using PlatformGfxDevice						= D3D12::D3D12GfxDevice;
	using PlatformTexture						= D3D12::D3D12Texture;
	using PlatformSwapChain						= D3D12::D3D12SwapChain;

}