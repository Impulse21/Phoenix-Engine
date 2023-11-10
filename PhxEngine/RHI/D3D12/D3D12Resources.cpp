#include "D3D12Resources.h"

#include "D3D12Context.h"

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;


bool D3D12Texture::CreateRenderTargetView(TextureDesc const& desc, D3D12_CPU_DESCRIPTOR_HANDLE& view) const
{
	return true;
}

bool D3D12Texture::CreateRenderTargetView(D3D12_RENDER_TARGET_VIEW_DESC const& d3d12Resc, D3D12_CPU_DESCRIPTOR_HANDLE& view)
{
	D3D12::Context::D3d12Device()->CreateRenderTargetView(this->D3D12Resource.Get(), &d3d12Resc, view);
	return true;
}