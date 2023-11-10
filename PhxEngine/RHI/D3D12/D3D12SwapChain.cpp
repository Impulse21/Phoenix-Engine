#include "D3D12SwapChain.h"

#include "RHI/RHIResources.h"
#include "RHI/D3D12/D3D12Context.h"
#include "DxgiFormatMapping.h"

using namespace PhxEngine;
const RHI::Texture& PhxEngine::RHI::D3D12::D3D12SwapChain::GetCurrentBackBuffer()
{
	return this->BackBuffers[this->NativeSwapchain4->GetCurrentBackBufferIndex()];
}
