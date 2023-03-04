#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "D3D12GraphicsDevice.h"

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

void PhxEngine::RHI::D3D12::D3D12GraphicsDevice::CreateViewport(ViewportDesc const& desc)
{
	assert(!this->m_activeViewport);
	if (this->m_activeViewport)
	{
		return;
	}

	assert(desc.WindowHandle);
	if (!desc.WindowHandle)
	{
		LOG_CORE_ERROR("Unable to create viewport. Invalid window handle");
		throw std::runtime_error("Invalid Window Error");
	}

	this->m_activeViewport = std::make_unique<D3D12Viewport>();
	auto& impl = *this->m_activeViewport;
	impl.Desc = desc;

	auto& mapping = GetDxgiFormatMapping(desc.Format);

	DXGI_SWAP_CHAIN_DESC1 dx12Desc = {};
	dx12Desc.Width = desc.Width;
	dx12Desc.Height = desc.Height;
	dx12Desc.Format = mapping.RtvFormat;
	dx12Desc.SampleDesc.Count = 1;
	dx12Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dx12Desc.BufferCount = desc.BufferCount;
	dx12Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dx12Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ThrowIfFailed(
		this->GetDxgiFactory()->CreateSwapChainForHwnd(
			this->GetGfxQueue()->GetD3D12CommandQueue(),
			static_cast<HWND>(desc.WindowHandle),
			&dx12Desc,
			nullptr,
			nullptr,
			impl.NativeSwapchain.ReleaseAndGetAddressOf()));

	ThrowIfFailed(impl.NativeSwapchain->QueryInterface(impl.NativeSwapchain4.ReleaseAndGetAddressOf()));

	// Create Back Buffer Textures
	CreateBackBuffers(this->GetActiveViewport());

	// Create a dummy Renderpass 
	if (!impl.RenderPass.IsValid())
	{
		// Create Dummy Render pass
		impl.RenderPass = this->CreateRenderPass({});
	}

	ThrowIfFailed(
		this->GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_frameFence)));

}

const ViewportDesc& PhxEngine::RHI::D3D12::D3D12GraphicsDevice::GetViewportDesc()
{
	return this->m_activeViewport->Desc;
}

void PhxEngine::RHI::D3D12::D3D12GraphicsDevice::ResizeViewport(ViewportDesc const desc)
{
	for (auto backBuffer : this->m_activeViewport->BackBuffers)
	{
		this->DeleteTexture(backBuffer);
	}
	this->DeleteRenderPass(this->m_activeViewport->RenderPass);

	// Wait for processing to finish
	this->WaitForIdle();

	this->m_activeViewport->Desc = desc;

	auto& mapping = GetDxgiFormatMapping(desc.Format);
	// Resize Swap Chain, Recreate BackBuffers
	this->m_activeViewport->NativeSwapchain4->ResizeBuffers(
		desc.BufferCount,
		desc.Width,
		desc.Height,
		mapping.RtvFormat,
		(UINT)0);

	this->CreateBackBuffers(this->GetActiveViewport());

	// Create a dummy Renderpass 
	if (!this->m_activeViewport->RenderPass.IsValid())
	{
		// Create Dummy Render pass
		this->m_activeViewport->RenderPass = this->CreateRenderPass({});
	}

}

void PhxEngine::RHI::D3D12::D3D12GraphicsDevice::CreateBackBuffers(D3D12Viewport* viewport)
{
	viewport->BackBuffers.resize(viewport->Desc.BufferCount);
	for (UINT i = 0; i < viewport->Desc.BufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(
			viewport->NativeSwapchain4->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		char allocatorName[32];
		sprintf_s(allocatorName, "Back Buffer %iu", i);

		viewport->BackBuffers[i] = this->CreateTexture(
			{
				.BindingFlags = BindingFlags::RenderTarget,
				.Dimension = TextureDimension::Texture2D,
				.Format = viewport->Desc.Format,
				.Width = viewport->Desc.Width,
				.Height = viewport->Desc.Height,
				.DebugName = std::string(allocatorName)
			},
			backBuffer);
	}

}

void PhxEngine::RHI::D3D12::D3D12GraphicsDevice::BeginFrame()
{
	assert(this->m_activeViewport);
	const UINT64 completedFrame = this->m_frameFence->GetCompletedValue();

	// If the fence is max uint64, that might indicate that the device has been removed as fences get reset in this case
	assert(completedFrame != UINT64_MAX);

	uint32_t bufferCount = this->m_activeViewport->Desc.BufferCount;
	// Since our frame count is 1 based rather then 0, increment the number of buffers by 1 so we don't have to wait on the first 3 frames
	// that are kicked off.
	if (this->m_frameCount >= (bufferCount + 1) && completedFrame < this->m_frameCount)
	{
		// Wait on the frames last value?
		// NULL event handle will simply wait immediately:
		//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion#remarks
		ThrowIfFailed(
			this->m_frameFence->SetEventOnCompletion(this->m_frameCount - bufferCount, NULL));
	}

	this->RunGarbageCollection(completedFrame);
}

void PhxEngine::RHI::D3D12::D3D12GraphicsDevice::EndFrame()
{
	HRESULT hr = this->m_activeViewport->NativeSwapchain4->Present(0, 0);

	// If the device was reset we must completely reinitialize the renderer.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		
#ifdef _DEBUG
		char buff[64] = {};
		sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
			static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) 
				? this->GetD3D12Device()->GetDeviceRemovedReason()
				: hr));
		OutputDebugStringA(buff);
#endif

		// TODO: Handle device lost
		// HandleDeviceLost();
	}

	// Mark complition of the graphics Queue 
	this->GetGfxQueue()->GetD3D12CommandQueue()->Signal(this->m_frameFence.Get(), this->m_frameCount);

	// Begin the next frame - this affects the GetCurrentBackBufferIndex()
	this->m_frameCount++;
}
