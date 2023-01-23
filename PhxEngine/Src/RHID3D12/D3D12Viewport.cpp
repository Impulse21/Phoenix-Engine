#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "D3D12Viewport.h"
#include "D3D12RHI.h"
#include "D3D12Adapter.h"
#include "D3D12Device.h"
#include "D3D12CommandQueue.h"

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

RHIViewportHandle D3D12RHI::CreateViewport(RHIViewportDesc const& desc)
{
	std::unique_ptr<D3D12Viewport> viewportImpl = std::make_unique<D3D12Viewport>(desc, this->GetAdapter());
	viewportImpl->Initialize();

	return RHIViewportHandle::Create(viewportImpl.release());
}

void PhxEngine::RHI::D3D12::D3D12Viewport::Initialize()
{
	D3D12RHI* rhi = this->GetParentRHI();
	IDXGIFactory2* factory6 = rhi->GetAdapter()->GetFactory6();

	assert(this->m_desc.WindowHandle);
	if (!this->m_desc.WindowHandle)
	{
		LOG_CORE_ERROR("Unable to create viewport. Invalid window handle");
		throw std::runtime_error("Invalid Window Error");
	}

	auto& mapping = GetDxgiFormatMapping(this->m_desc.Format);

	DXGI_SWAP_CHAIN_DESC1 dx12Desc = {};
	dx12Desc.Width = this->m_desc.Width;
	dx12Desc.Height = this->m_desc.Height;
	dx12Desc.Format = mapping.RtvFormat;
	dx12Desc.SampleDesc.Count = 1;
	dx12Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dx12Desc.BufferCount = this->m_desc.BufferCount;
	dx12Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dx12Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ThrowIfFailed(
		factory6->CreateSwapChainForHwnd(
			rhi->GetAdapter()->GetDevice()->GetGfxQueue()->GetD3D12CommandQueue(),
			static_cast<HWND>(this->m_desc.WindowHandle),
			&dx12Desc,
			nullptr,
			nullptr,
			this->m_swapchain.ReleaseAndGetAddressOf()));

	ThrowIfFailed(this->m_swapchain->QueryInterface(this->m_swapchain4.ReleaseAndGetAddressOf()));

	// Create Back Buffer Textures

	this->m_backBuffers.resize(this->m_desc.BufferCount);
	for (UINT i = 0; i < this->m_desc.BufferCount; i++)
	{
		RefCountPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(
			this->m_swapchain4->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		char allocatorName[32];
		sprintf_s(allocatorName, "Back Buffer %iu", i);

		this->m_backBuffers[i] = rhi->CreateTexture(
			{
				.Dimension = TextureDimension::Texture2D,
				.Format = this->m_desc.Format,
				.Width = this->m_desc.Width,
				.Height = this->m_desc.Height,
				.DebugName = std::string(allocatorName)
			},
			backBuffer);
	}

	// Create a dummy Renderpass 
	if (!this->m_renderPass)
	{
		// Create Dummy Render pass
		this->m_renderPass = rhi->CreateRenderPass({});
	}

	ThrowIfFailed(
		rhi->GetAdapter()->GetRootDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_frameFence)));

}

void PhxEngine::RHI::D3D12::D3D12Viewport::WaitForIdleFrame()
{
	const UINT64 completedFrame = this->m_frameFence->GetCompletedValue();

	// If the fence is max uint64, that might indicate that the device has been removed as fences get reset in this case
	assert(completedFrame != UINT64_MAX);

	// Since our frame count is 1 based rather then 0, increment the number of buffers by 1 so we don't have to wait on the first 3 frames
	// that are kicked off.
	if (this->m_frameCount >= (this->m_desc.BufferCount + 1) && completedFrame < this->m_frameCount)
	{
		// Wait on the frames last value?
		// NULL event handle will simply wait immediately:
		//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion#remarks
		ThrowIfFailed(
			this->m_frameFence->SetEventOnCompletion(this->m_frameCount - this->m_desc.BufferCount, NULL));
	}
}

void PhxEngine::RHI::D3D12::D3D12Viewport::Present()
{
	HRESULT hr = this->m_swapchain4->Present(0, 0);

	// If the device was reset we must completely reinitialize the renderer.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		
#ifdef _DEBUG
		char buff[64] = {};
		sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
			static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) 
				? this->ParentAdapter->GetRootDevice()->GetDeviceRemovedReason() 
				: hr));
		OutputDebugStringA(buff);
#endif

		// TODO: Handle device lost
		// HandleDeviceLost();
	}

	// Mark complition of the graphics Queue 
	this->ParentAdapter->GetDevice()->GetGfxQueue()->GetD3D12CommandQueue()->Signal(this->m_frameFence.Get(), this->m_frameCount);

	// Begin the next frame - this affects the GetCurrentBackBufferIndex()
	this->m_frameCount++;
}
