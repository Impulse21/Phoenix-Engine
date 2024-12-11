#include "pch.h"

#include "phx/rhi/dx12/Dx12CommandListRecorder.h"
#include "phx/rhi/dx12/Dx12GfxDevice.h"

using namespace phx;
using namespace phx::rhi;

void dx12::GfxCommandListRecorder::Open(CommandListResource* resource)
{
	if (resource->Allocator)
		return; // Already open

	auto* device = dx12::GfxDeviceDx12::Instance();

	D3D12CommandQueue& queue = device->GetQueue(resource->Type);
	resource->Allocator = queue.RequestAllocator();

	if (resource->CmdList == nullptr)
	{
		device->GetD3D12Device()->CreateCommandList(
			0,
			queue.Type,
			resource->Allocator,
			nullptr,
			IID_PPV_ARGS(&resource->CmdList));

		resource->CmdList->SetName(L"GfxDeviceD3D12::CommandList");
		ThrowIfFailed(
			resource->CmdList.As<ID3D12GraphicsCommandList6>(
				&resource->CmdList6));
	}
	else
	{
		resource->CmdList->Reset(resource->Allocator, nullptr);
	}

	// Bind Heaps
	std::array<ID3D12DescriptorHeap*, 2> heaps;
	Span<GpuDescriptorHeap> gpuHeaps = device->GetGpuDescriptorHeaps();
	for (int i = 0; i < gpuHeaps.Size(); i++)
	{
		heaps[i] = gpuHeaps[i].GetNativeHeap();
	}

	resource->CmdList6->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
}

void dx12::GfxCommandListRecorder::Close()
{
	m_commandList->Close();
}

void dx12::GfxCommandListRecorder::BeginMarker(const char*)
{
}

void dx12::GfxCommandListRecorder::EndMarker()
{

}

void dx12::GfxCommandListRecorder::RenderPassBegin(SwapChainBindings* bindings)
{
	m_numRenderPasses = 0;

	ID3D12Resource* swapChainImage = bindings->FrameBackBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE view = bindings->FrameBackBufferRTV;

#if false // Enhanced Barriers
	// Define the texture barrier
	D3D12_BARRIER_TEXTURE_BARRIER textureBarrier = {};
	textureBarrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
	textureBarrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
	textureBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;

	textureBarrier.SyncAfter = D3D12_BARRIER_SYNC_PIXEL_SHADER;
	textureBarrier.AccessAfter = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
	textureBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;

	textureBarrier.pResource = pTexture;  // The texture resource to transition
	textureBarrier.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE(D3D12_BARRIER_SUBRESOURCE_RANGE_FLAG_ALL_SUBRESOURCES);

	// Describe the barrier type
	D3D12_BARRIER_GROUP barrierGroup = {};
	barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
	barrierGroup.NumBarriers = 1;
	barrierGroup.pTextureBarriers = &textureBarrier;

#else
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainImage;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandList->ResourceBarrier(1, &barrier);
	ClearTexture(view, bindings->ClearColour->Colour);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	m_renderPassBarriers[m_numRenderPasses++] = barrier;
#endif

	m_commandList->OMSetRenderTargets(1, &view, 0, nullptr);
}

void dx12::GfxCommandListRecorder::ClearTexture(D3D12_CPU_DESCRIPTOR_HANDLE view, phx::rhi::Color const& clearColour)
{
	m_commandList->ClearRenderTargetView(
		view,
		&clearColour.R,
		0,
		nullptr);
}

void dx12::GfxCommandListRecorder::RenderPassEnd()
{
	m_commandList->ResourceBarrier((UINT)m_numRenderPasses, m_renderPassBarriers.data());
	m_numRenderPasses = 0;
}

void SetViewports(phx::Span<rhi::Viewport> viewports)
{
	CD3DX12_VIEWPORT dx12Viewports[16] = {};
	for (int i = 0; i < viewports.Size(); i++)
	{
		const Viewport& viewport = viewports[i];
		dx12Viewports[i] = CD3DX12_VIEWPORT(
			viewport.MinX,
			viewport.MinY,
			viewport.GetWidth(),
			viewport.GetHeight(),
			viewport.MinZ,
			viewport.MaxZ);
	}

	this->m_commandList->RSSetViewports((UINT)viewports.Size(), dx12Viewports);

}

void SetScissors(phx::Span<Rect> scissors)
{
	CD3DX12_RECT dx12Scissors[16] = {};
	for (int i = 0; i < scissors.Size(); i++)
	{
		const Rect& scissor = scissors[i];

		dx12Scissors[i] = CD3DX12_RECT(
			scissor.MinX,
			scissor.MinY,
			scissor.MaxX,
			scissor.MaxY);
	}

	this->m_commandList->RSSetScissorRects((UINT)scissors.Size(), dx12Scissors);

}

void SetPipelineState(dx12::PipelineStateResource* resource)
{
	m_activePipelineType = resource->Type;
	this->m_commandList->SetPipelineState(resource->D3D12PipelineState.Get());
	this->m_commandList->SetGraphicsRootSignature(resource->RootSignature.Get());

	this->m_commandList->IASetPrimitiveTopology(resource->Topology);

}

void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0)
{
	this->m_commandList->DrawIndexedInstanced(
		indexCount,
		instanceCount,
		startIndex,
		baseVertex,
		startInstance);
}

void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0)
{
	this->m_commandList->DrawInstanced(
		vertexCount,
		instanceCount,
		startVertex,
		startInstance);
}

void SetDynamicVertexBuffer(size_t offset, uint32_t slot, size_t numVertices, size_t vertexSize)
{
	UNREFERENCED_PARAMETER(tempBuffer);
	UNREFERENCED_PARAMETER(offset);
	UNREFERENCED_PARAMETER(slot);
	UNREFERENCED_PARAMETER(numVertices);
	UNREFERENCED_PARAMETER(vertexSize);
}

void SetDynamicIndexBuffer(size_t offset, size_t numIndicies, Format indexFormat)
{
	UNREFERENCED_PARAMETER(tempBuffer);
	UNREFERENCED_PARAMETER(offset);
	UNREFERENCED_PARAMETER(numIndicies);
	UNREFERENCED_PARAMETER(indexFormat);
}

void SetPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants)
{
	if (this->m_activePipelineType == PipelineStateResource::PipelineType::Compute)
	{
		this->m_commandList->SetComputeRoot32BitConstants(rootParameterIndex, sizeInBytes / sizeof(uint32_t), constants, 0);
	}
	else
	{
		this->m_commandList->SetGraphicsRoot32BitConstants(rootParameterIndex, sizeInBytes / sizeof(uint32_t), constants, 0);
	}
}