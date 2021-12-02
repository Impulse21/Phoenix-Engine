#include "CommandList.h"

#include "GraphicsDevice.h"
#include <pix.h>

#include <PhxEngine/Core/SafeCast.h>

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::Dx12;

namespace
{
	D3D12_COMMAND_LIST_TYPE Convert(PhxEngine::RHI::CommandQueueType queueType)
	{
        D3D12_COMMAND_LIST_TYPE d3dCommandListType;
        switch (queueType)
        {
        case PhxEngine::RHI::CommandQueueType::Compute:
            d3dCommandListType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            break;
        case PhxEngine::RHI::CommandQueueType::Copy:
            d3dCommandListType = D3D12_COMMAND_LIST_TYPE_COPY;
            break;

        case PhxEngine::RHI::CommandQueueType::Graphics:
        default:
            d3dCommandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
        }

        return d3dCommandListType;
	}
}

PhxEngine::RHI::Dx12::CommandList::CommandList(GraphicsDevice& graphicsDevice, CommandListDesc const& desc)
	: m_graphicsDevice(graphicsDevice)
	, m_desc(desc)
	, m_commandAlloatorPool(graphicsDevice.GetD3D12Device2(), Convert(desc.QueueType))
{}

void PhxEngine::RHI::Dx12::CommandList::Open()
{
	PHX_ASSERT(!this->m_activeD3D12CommandAllocator);

    this->m_trackedData = std::make_shared<TrackedResources>();

    auto lastCompletedFence = this->m_graphicsDevice.GetGfxQueue()->GetLastCompletedFence();

    this->m_activeD3D12CommandAllocator = this->m_commandAlloatorPool.RequestAllocator(lastCompletedFence);

    if (this->m_d3d12CommandList)
    {
        this->m_d3d12CommandList->Reset(this->m_activeD3D12CommandAllocator, nullptr);
    }
    else
    {
        auto type = Convert(this->m_desc.QueueType);
        this->m_graphicsDevice.GetD3D12Device2()->CreateCommandList(
            0,
            type,
            this->m_activeD3D12CommandAllocator,
            nullptr,
            IID_PPV_ARGS(&this->m_d3d12CommandList));
    }

    this->m_d3d12CommandList->QueryInterface(&this->m_d3d12CommnadList4);


}

void PhxEngine::RHI::Dx12::CommandList::Close()
{
    // Is this needed
    PHX_ASSERT(this->m_d3d12CommandList);

    this->m_d3d12CommandList->Close();
}

ScopedMarker CommandList::BeginScropedMarker(std::string name)
{
    this->BeginMarker(name);
    return ScopedMarker(this);
}

void CommandList::BeginMarker(std::string name)
{
    PIXBeginEvent(this->m_d3d12CommandList, 0, std::wstring(name.begin(), name.end()).c_str());
}

void CommandList::EndMarker()
{
    PIXEndEvent(this->m_d3d12CommandList);
}

void CommandList::TransitionBarrier(
    ITexture* texture,
    ResourceStates beforeState,
    ResourceStates afterState)
{
    Texture* textureImpl = SafeCast<Texture*>(texture);
    PHX_ASSERT(textureImpl);

    this->TransitionBarrier(
        textureImpl->D3D12Resource,
        ConvertResourceStates(beforeState),
        ConvertResourceStates(afterState));

    this->m_trackedData->Resource.push_back(texture);
}

void PhxEngine::RHI::Dx12::CommandList::TransitionBarrier(
    IBuffer* buffer,
    ResourceStates beforeState,
    ResourceStates afterState)
{
    GpuBuffer* bufferImpl = SafeCast<GpuBuffer*>(buffer);
    PHX_ASSERT(bufferImpl);

    this->TransitionBarrier(
        bufferImpl->D3D12Resource,
        ConvertResourceStates(beforeState),
        ConvertResourceStates(afterState));

    this->m_trackedData->Resource.push_back(buffer);
}

void CommandList::ClearTextureFloat(ITexture* texture, Color const& clearColour)
{
    Texture* textureImpl = SafeCast<Texture*>(texture);
    PHX_ASSERT(textureImpl);

    this->m_trackedData->Resource.push_back(texture);

    PHX_ASSERT(!textureImpl->RtvAllocation.IsNull())
    this->m_d3d12CommandList->ClearRenderTargetView(
        textureImpl->RtvAllocation.GetCpuHandle(),
        &clearColour.R,
        0,
        nullptr);
}

void CommandList::ClearDepthStencilTexture(
    ITexture* depthStencil,
    bool clearDepth,
    float depth,
    bool clearStencil,
    uint8_t stencil)
{
    Texture* textureImpl = SafeCast<Texture*>(depthStencil);
    PHX_ASSERT(textureImpl);

    D3D12_CLEAR_FLAGS flags = {};
    if (clearDepth)
    {
        flags |= D3D12_CLEAR_FLAG_DEPTH;
    }

    if (clearStencil)
    {
        flags |= D3D12_CLEAR_FLAG_STENCIL;
    }

    PHX_ASSERT(!textureImpl->DsvAllocation.IsNull())
    this->m_d3d12CommandList->ClearDepthStencilView(
        textureImpl->DsvAllocation.GetCpuHandle(),
        flags,
        depth,
        stencil,
        0,
        nullptr);

    this->m_trackedData->Resource.push_back(depthStencil);
}

std::shared_ptr<TrackedResources> PhxEngine::RHI::Dx12::CommandList::Executed(uint64_t fenceValue)
{
    this->m_commandAlloatorPool.DiscardAllocator(fenceValue, this->m_activeD3D12CommandAllocator);
    this->m_activeD3D12CommandAllocator = nullptr;

    auto trackedResources = this->m_trackedData;
    this->m_trackedData.reset();

    return trackedResources;
}

void CommandList::TransitionBarrier(RefCountPtr<ID3D12Resource> d3d12Resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        d3d12Resource,
        beforeState, afterState);

    // TODO: Batch Barrier
    this->m_d3d12CommandList->ResourceBarrier(1, &barrier);
}
