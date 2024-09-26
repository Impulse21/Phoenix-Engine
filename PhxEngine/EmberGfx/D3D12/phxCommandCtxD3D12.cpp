#include "pch.h"
#include "phxCommandCtxD3D12.h"

#include "phxGfxDeviceD3D12.h"

using namespace phx::gfx;

using namespace phx::gfx::platform;

#ifdef USING_D3D12_AGILITY_SDK
extern "C"
{
	// Used to enable the "Agility SDK" components
	__declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
	__declspec(dllexport) extern const char8_t* D3D12SDKPath = u8".\\D3D12\\";
}
#endif

void CommandCtxD3D12::Reset(size_t id, CommandQueueType queueType, GfxDeviceD3D12* device)
{
	ID3D12Device* d3d12Device = device->GetD3D12Device();
	D3D12CommandQueue& queue = device->GetQueue(queueType);

	this->m_allocator = nullptr;
	this->m_allocator = queue.RequestAllocator();
	if (this->m_commandList == nullptr)
	{
		d3d12Device->CreateCommandList(
			0,
			queue.Type,
			this->m_allocator,
			nullptr,
			IID_PPV_ARGS(&this->m_commandList));

		this->m_commandList->SetName(L"GfxDeviceD3D12::CommandList");
		ThrowIfFailed(
			this->m_commandList.As<ID3D12GraphicsCommandList6>(
				&this->m_commandList6));

	}
	else
	{
		this->m_commandList->Reset(this->m_allocator, nullptr);
	}

	this->m_id = id;
	this->m_queueType = queueType;
	this->m_waits.clear();
	this->m_isWaitedOn.store(false);

	// Bind Heaps
	std::array<ID3D12DescriptorHeap*, 2> heaps;
	Span<GpuDescriptorHeap> gpuHeaps = device->GetGpuDescriptorHeaps();
	for (int i = 0; i < gpuHeaps.Size(); i++)
	{
		heaps[i] = gpuHeaps[i].GetNativeHeap();
	}

	this->m_commandList6->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
}

void CommandCtxD3D12::TransitionBarrier(GpuBarrier const& barrier)
{
}

void CommandCtxD3D12::TransitionBarriers(Span<GpuBarrier> gpuBarriers)
{

}

void CommandCtxD3D12::ClearBackBuffer(Color const& clearColour)
{
	GfxDeviceD3D12* device = GfxDeviceD3D12::Instance();
	auto view = device->GetBackBuffer();

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = device->GetBackBuffer();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	this->m_commandList->ResourceBarrier(1, &barrier);
	this->m_commandList->ClearRenderTargetView(
		device->GetBackBufferView(),
		&clearColour.R,
		0,
		nullptr);


	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	this->m_commandList->ResourceBarrier(1, &barrier);
}

void CommandCtxD3D12::ClearTextureFloat(TextureHandle texture, Color const& clearColour)
{
}

void CommandCtxD3D12::ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil)
{
}
