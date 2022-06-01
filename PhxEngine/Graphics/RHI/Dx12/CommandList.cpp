#include "phxpch.h"
#include "CommandList.h"

#include <pix.h>

#include "GraphicsDevice.h"
#include "DescriptorHeap.h"
#include "BiindlessDescriptorTable.h"
#include "UploadBuffer.h"

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

    // helper function for texture subresource calculations
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dn705766(v=vs.85).aspx
    uint32_t CalcSubresource(uint32_t mipSlice, uint32_t arraySlice, uint32_t planeSlice, uint32_t mipLevels, uint32_t arraySize)
    {
        return mipSlice + (arraySlice * mipLevels) + (planeSlice * mipLevels * arraySize);
    }
}

PhxEngine::RHI::Dx12::CommandList::CommandList(GraphicsDevice& graphicsDevice, CommandListDesc const& desc)
	: m_graphicsDevice(graphicsDevice)
	, m_desc(desc)
	, m_commandAlloatorPool(graphicsDevice.GetD3D12Device2(), Convert(desc.QueueType))
    , m_uploadBuffer(std::make_unique<UploadBuffer>(graphicsDevice.GetD3D12Device2()))
    , m_dynamicSubAllocatorPool(*graphicsDevice.GetResourceGpuHeap(), DynamicChunkSizeSrvUavCbv)
{
}

PhxEngine::RHI::Dx12::CommandList::~CommandList()
{
    this->m_d3d12CommandList.Reset();
}

void PhxEngine::RHI::Dx12::CommandList::Open()
{
	assert(!this->m_activeD3D12CommandAllocator);

    this->m_trackedData = std::make_shared<TrackedResources>();

    auto lastCompletedFence = this->m_graphicsDevice.GetGfxQueue()->GetLastCompletedFence();

    this->m_activeD3D12CommandAllocator = this->m_commandAlloatorPool.RequestAllocator(lastCompletedFence);

    this->m_activeDynamicSubAllocator = this->m_dynamicSubAllocatorPool.RequestAllocator(lastCompletedFence);
    this->m_activeDynamicSubAllocator->ReleaseAllocations();

    if (this->m_d3d12CommandList)
    {
        this->m_d3d12CommandList->Reset(this->m_activeD3D12CommandAllocator.Get(), nullptr);
    }
    else
    {
        auto type = Convert(this->m_desc.QueueType);
        this->m_graphicsDevice.GetD3D12Device2()->CreateCommandList(
            0,
            type,
            this->m_activeD3D12CommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&this->m_d3d12CommandList));

        this->m_d3d12CommandList->SetName(std::wstring(this->m_desc.DebugName.begin(), this->m_desc.DebugName.end()).c_str());

    }
    // This might be a problem as it might delete any pending resources.
    this->m_uploadBuffer->Reset();

    // Bind Heaps
    this->SetDescritporHeaps(
        {
            this->m_graphicsDevice.GetResourceGpuHeap(),
            this->m_graphicsDevice.GetSamplerGpuHeap()
        });

}

void PhxEngine::RHI::Dx12::CommandList::Close()
{
    // Is this needed
    assert(this->m_d3d12CommandList);

    this->m_d3d12CommandList->Close();
}

ScopedMarker CommandList::BeginScopedMarker(std::string name)
{
    this->BeginMarker(name);
    return ScopedMarker(this);
}

void CommandList::BeginMarker(std::string name)
{
    PIXBeginEvent(this->m_d3d12CommandList.Get(), 0, std::wstring(name.begin(), name.end()).c_str());
}

void CommandList::EndMarker()
{
    PIXEndEvent(this->m_d3d12CommandList.Get());
}

void CommandList::TransitionBarrier(
    TextureHandle texture,
    ResourceStates beforeState,
    ResourceStates afterState)
{
    auto textureImpl = std::static_pointer_cast<Texture>(texture);
    assert(textureImpl);

    this->TransitionBarrier(
        textureImpl->D3D12Resource,
        ConvertResourceStates(beforeState),
        ConvertResourceStates(afterState));

    this->m_trackedData->Resource.push_back(texture);
}

void PhxEngine::RHI::Dx12::CommandList::TransitionBarrier(
    BufferHandle buffer,
    ResourceStates beforeState,
    ResourceStates afterState)
{
    auto bufferImpl = std::static_pointer_cast <GpuBuffer>(buffer);
    assert(bufferImpl);

    this->TransitionBarrier(
        bufferImpl->D3D12Resource,
        ConvertResourceStates(beforeState),
        ConvertResourceStates(afterState));

    this->m_trackedData->Resource.push_back(buffer);
}

void CommandList::ClearTextureFloat(TextureHandle texture, Color const& clearColour)
{
    auto textureImpl = std::static_pointer_cast<Texture>(texture);
    assert(textureImpl);

    this->m_trackedData->Resource.push_back(texture);

    assert(!textureImpl->RtvAllocation.IsNull());
    this->m_d3d12CommandList->ClearRenderTargetView(
        textureImpl->RtvAllocation.GetCpuHandle(),
        &clearColour.R,
        0,
        nullptr);
}

void CommandList::ClearDepthStencilTexture(
    TextureHandle depthStencil,
    bool clearDepth,
    float depth,
    bool clearStencil,
    uint8_t stencil)
{
    auto textureImpl = std::static_pointer_cast<Texture>(depthStencil);
    assert(textureImpl);

    D3D12_CLEAR_FLAGS flags = {};
    if (clearDepth)
    {
        flags |= D3D12_CLEAR_FLAG_DEPTH;
    }

    if (clearStencil)
    {
        flags |= D3D12_CLEAR_FLAG_STENCIL;
    }

    assert(!textureImpl->DsvAllocation.IsNull());
    this->m_d3d12CommandList->ClearDepthStencilView(
        textureImpl->DsvAllocation.GetCpuHandle(),
        flags,
        depth,
        stencil,
        0,
        nullptr);

    this->m_trackedData->Resource.push_back(depthStencil);
}

void CommandList::Draw(
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t startVertex,
    uint32_t startInstance)
{
    this->m_d3d12CommandList->DrawInstanced(
        vertexCount,
        instanceCount,
        startVertex,
        startInstance);
}

void CommandList::DrawIndexed(
    uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t startIndex,
    int32_t baseVertex,
    uint32_t startInstance)
{
    this->m_d3d12CommandList->DrawIndexedInstanced(
        indexCount,
        instanceCount,
        startIndex,
        baseVertex,
        startInstance);
}

void CommandList::WriteBuffer(BufferHandle buffer, const void* data, size_t dataSize, uint64_t destOffsetBytes)
{
    auto heapAllocation = this->m_uploadBuffer->Allocate(dataSize, buffer->GetDesc().StrideInBytes);
    memcpy(heapAllocation.Cpu, data, dataSize);
    auto bufferImpl = std::static_pointer_cast<GpuBuffer>(buffer);
    this->m_d3d12CommandList->CopyBufferRegion(
        bufferImpl->D3D12Resource.Get(),
        destOffsetBytes,
        heapAllocation.D3D12Resouce,
        heapAllocation.Offset,
        dataSize);

    // TODO: See how the upload buffer can be used here
    /*
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
    ThrowIfFailed(
        this->m_graphicsDevice.GetD3D12Device2()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(dataSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&intermediateResource)));

    D3D12_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pData = data;
    subresourceData.RowPitch = dataSize;
    subresourceData.SlicePitch = subresourceData.RowPitch;

    GpuBuffer* bufferImpl = SafeCast<GpuBuffer*>(buffer.Get());

    UpdateSubresources(
        this->m_d3d12CommandList.Get(),
        bufferImpl->D3D12Resource,
        intermediateResource,
        0, 0, 1, &subresourceData);
        this->m_trackedData->NativeResources.push_back(intermediateResource);
    */
    
    this->m_trackedData->Resource.push_back(buffer);

}

void PhxEngine::RHI::Dx12::CommandList::WriteTexture(TextureHandle texture, uint32_t firstSubresource, size_t numSubresources, SubresourceData* pSubresourceData)
{
    auto textureImpl = std::static_pointer_cast<Texture>(texture);
    UINT64 requiredSize = GetRequiredIntermediateSize(textureImpl->D3D12Resource.Get(), firstSubresource, numSubresources);

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
    ThrowIfFailed(
        this->m_graphicsDevice.GetD3D12Device2()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&intermediateResource)));


    std::vector<D3D12_SUBRESOURCE_DATA> subresources(numSubresources);
    for (int i = 0; i < numSubresources; ++i)
    {
        auto& subresource = subresources[i];
        subresource.RowPitch = pSubresourceData[i].rowPitch;
        subresource.SlicePitch = pSubresourceData[i].slicePitch;
        subresource.pData = pSubresourceData[i].pData;
    }

    UpdateSubresources(
        this->m_d3d12CommandList.Get(),
        textureImpl->D3D12Resource.Get(),
        intermediateResource.Get(),
        0,
        firstSubresource,
        subresources.size(),
        subresources.data());

    this->m_trackedData->Resource.push_back(texture);
    this->m_trackedData->NativeResources.push_back(intermediateResource);
}

void PhxEngine::RHI::Dx12::CommandList::WriteTexture(TextureHandle texture, uint32_t arraySlice, uint32_t mipLevel, const void* data, size_t rowPitch, size_t depthPitch)
{
    // LOG_CORE_FATAL("NOT IMPLEMENTED FUNCTION CALLED: CommandList::WriteTexture");
    assert(false);
    auto textureImpl = std::static_pointer_cast<Texture>(texture);
    uint32_t subresource = CalcSubresource(mipLevel, arraySlice, 0, textureImpl->Desc.MipLevels, textureImpl->Desc.ArraySize);

    D3D12_RESOURCE_DESC resourceDesc = textureImpl->D3D12Resource->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    uint32_t numRows;
    uint64_t rowSizeInBytes;
    uint64_t totalBytes;
}

void PhxEngine::RHI::Dx12::CommandList::SetRenderTargets(std::vector<TextureHandle> const& renderTargets, TextureHandle depthStencil)
{
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetViews(renderTargets.size());
    for (int i = 0; i < renderTargets.size(); i++)
    {
        auto textureImpl = std::static_pointer_cast<Texture>(renderTargets[i]);
        renderTargetViews[i] = textureImpl->RtvAllocation.GetCpuHandle();

        this->m_trackedData->Resource.push_back(renderTargets[i]);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE depthView = {};
    bool hasDepth = depthStencil != nullptr;
    if (hasDepth)
    {
        auto textureImpl = std::static_pointer_cast<Texture>(depthStencil);
        depthView = textureImpl->DsvAllocation.GetCpuHandle();
    }

    this->m_d3d12CommandList->OMSetRenderTargets(
        renderTargetViews.size(),
        renderTargetViews.data(),
        hasDepth,
        hasDepth ? &depthView : nullptr);
}

void PhxEngine::RHI::Dx12::CommandList::SetGraphicsPSO(GraphicsPSOHandle graphisPSO)
{
    auto graphicsPsoImpl = std::static_pointer_cast<GraphicsPSO>(graphisPSO);
    this->m_d3d12CommandList->SetPipelineState(graphicsPsoImpl->D3D12PipelineState.Get());

    this->m_d3d12CommandList->SetGraphicsRootSignature(graphicsPsoImpl->RootSignature.Get());

    const auto& desc = graphicsPsoImpl->GetDesc();
    switch (desc.PrimType)
    {
    case PrimitiveType::TriangleList:
        this->m_d3d12CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        break;

    default:
        assert(true);
    }

    this->m_trackedData->Resource.push_back(graphisPSO);
}

void PhxEngine::RHI::Dx12::CommandList::SetViewports(Viewport* viewports, size_t numViewports)
{
    CD3DX12_VIEWPORT dx12Viewports[16] = {};
    for (int i = 0; i < numViewports; i++)
    {
        Viewport& viewport = viewports[i];
        dx12Viewports[i] = CD3DX12_VIEWPORT(
            viewport.MinX,
            viewport.MinY,
            viewport.GetWidth(),
            viewport.GetHeight(),
            viewport.MinZ,
            viewport.MaxZ);
    }

    this->m_d3d12CommandList->RSSetViewports((UINT)numViewports, dx12Viewports);
}

void PhxEngine::RHI::Dx12::CommandList::SetScissors(Rect* scissors, size_t numScissors)
{
    CD3DX12_RECT dx12Scissors[16] = {};
    for (int i = 0; i < numScissors; i++)
    {
        const Rect& scissor = scissors[i];

        dx12Scissors[i] = CD3DX12_RECT(
            scissor.MinX,
            scissor.MinY,
            scissor.MaxX,
            scissor.MaxY);
    }

    this->m_d3d12CommandList->RSSetScissorRects((UINT)numScissors, dx12Scissors);
}

std::shared_ptr<TrackedResources> CommandList::Executed(uint64_t fenceValue)
{
    this->m_commandAlloatorPool.DiscardAllocator(fenceValue, this->m_activeD3D12CommandAllocator.Get());
    this->m_activeD3D12CommandAllocator = nullptr;

    this->m_dynamicSubAllocatorPool.DiscardAllocator(fenceValue, this->m_activeDynamicSubAllocator);
    this->m_activeDynamicSubAllocator = nullptr;

    auto trackedResources = this->m_trackedData;
    this->m_trackedData.reset();

    return trackedResources;
}

void PhxEngine::RHI::Dx12::CommandList::SetDescritporHeaps(std::array<GpuDescriptorHeap*, 2> const& shaderHeaps)
{
    std::vector<ID3D12DescriptorHeap*> heaps;
    for (auto* heap : shaderHeaps)
    {
        if (heap)
        {
            heaps.push_back(heap->GetNativeHeap());
        }
    }

    this->m_d3d12CommandList->SetDescriptorHeaps(heaps.size(), heaps.data());
}

void CommandList::TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        d3d12Resource.Get(),
        beforeState, afterState);

    // TODO: Batch Barrier
    this->m_d3d12CommandList->ResourceBarrier(1, &barrier);
}

void PhxEngine::RHI::Dx12::CommandList::BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants)
{
    this->m_d3d12CommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, sizeInBytes / sizeof(uint32_t), constants, 0);
}

void PhxEngine::RHI::Dx12::CommandList::BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
    UploadBuffer::Allocation alloc = this->m_uploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    std::memcpy(alloc.Cpu, bufferData, sizeInBytes);

    this->m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, alloc.Gpu);
}

void PhxEngine::RHI::Dx12::CommandList::BindVertexBuffer(uint32_t slot, BufferHandle vertexBuffer)
{
    auto vertexBufferImpl = std::static_pointer_cast<GpuBuffer>(vertexBuffer);

    this->m_d3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferImpl->VertexView);
    this->m_trackedData->Resource.push_back(vertexBuffer);
}

void PhxEngine::RHI::Dx12::CommandList::BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
{
    size_t bufferSize = numVertices * vertexSize;
    auto heapAllocation = this->m_uploadBuffer->Allocate(bufferSize, vertexSize);
    memcpy(heapAllocation.Cpu, vertexBufferData, bufferSize);

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = heapAllocation.Gpu;
    vertexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
    vertexBufferView.StrideInBytes = static_cast<UINT>(vertexSize);

    this->m_d3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void PhxEngine::RHI::Dx12::CommandList::BindIndexBuffer(BufferHandle indexBuffer)
{
    auto bufferImpl = std::static_pointer_cast<GpuBuffer>(indexBuffer);

    this->m_d3d12CommandList->IASetIndexBuffer(&bufferImpl->IndexView);
    this->m_trackedData->Resource.push_back(indexBuffer);
}

void PhxEngine::RHI::Dx12::CommandList::BindDynamicIndexBuffer(size_t numIndicies, FormatType indexFormat, const void* indexBufferData)
{
    size_t indexSizeInBytes = indexFormat == FormatType::R16_UINT ? 2 : 4;
    size_t bufferSize = numIndicies * indexSizeInBytes;

    auto heapAllocation = this->m_uploadBuffer->Allocate(bufferSize, indexSizeInBytes);
    memcpy(heapAllocation.Cpu, indexBufferData, bufferSize);

    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.BufferLocation = heapAllocation.Gpu;
    indexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
    const auto& formatMapping = GetDxgiFormatMapping(indexFormat);;
    
    indexBufferView.Format = formatMapping.srvFormat;

    this->m_d3d12CommandList->IASetIndexBuffer(&indexBufferView);
}

void PhxEngine::RHI::Dx12::CommandList::BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData)
{
    size_t sizeInBytes = numElements * elementSize;
    UploadBuffer::Allocation alloc = this->m_uploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    std::memcpy(alloc.Cpu, bufferData, sizeInBytes);

    this->m_d3d12CommandList->SetGraphicsRootShaderResourceView(rootParameterIndex, alloc.Gpu);
}

void PhxEngine::RHI::Dx12::CommandList::BindStructuredBuffer(size_t rootParameterIndex, BufferHandle buffer)
{
    auto bufferImpl = std::static_pointer_cast<GpuBuffer>(buffer);

    this->m_d3d12CommandList->SetGraphicsRootShaderResourceView(
        rootParameterIndex,
        bufferImpl->D3D12Resource->GetGPUVirtualAddress());
}

void PhxEngine::RHI::Dx12::CommandList::BindResourceTable(size_t rootParameterIndex)
{
    if (this->m_graphicsDevice.GetMinShaderModel() < ShaderModel::SM_6_6)
    {
        this->m_d3d12CommandList->SetGraphicsRootDescriptorTable(
            rootParameterIndex,
            this->m_graphicsDevice.GetBindlessTable()->GetGpuHandle(0));
    }
}

void PhxEngine::RHI::Dx12::CommandList::BindSamplerTable(size_t rootParameterIndex)
{
    // TODO:
    assert(false);
}

void CommandList::BindDynamicDescriptorTable(size_t rootParameterIndex, std::vector<TextureHandle> const& textures)
{
    // Request Descriptoprs for table
    // Validate with Root Signature. Maybe an improvment in the future.
    DescriptorHeapAllocation descriptorTable = this->m_activeDynamicSubAllocator->Allocate(textures.size());
    for (int i = 0; i < textures.size(); i++)
    {
        auto textureImpl = std::static_pointer_cast<Texture>(textures[i]);
        this->m_graphicsDevice.GetD3D12Device2()->CopyDescriptorsSimple(
            1,
            descriptorTable.GetCpuHandle(i),
            textureImpl->SrvAllocation.GetCpuHandle(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        this->m_trackedData->Resource.push_back(textures[i]);
    }

    this->m_d3d12CommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
}

DynamicSubAllocatorPool::DynamicSubAllocatorPool(GpuDescriptorHeap& gpuDescriptorHeap, uint32_t chunkSize)
    : m_chunkSize(chunkSize)
    , m_gpuDescriptorHeap(gpuDescriptorHeap)
{
}

DynamicSuballocator* DynamicSubAllocatorPool::RequestAllocator(uint64_t completedFenceValue)
{
    DynamicSuballocator* pAllocator = nullptr;
    if (!this->m_availableAllocators.empty())
    {
        auto& allocatorPair = this->m_availableAllocators.front();
        if (allocatorPair.first <= completedFenceValue)
        {
            pAllocator = allocatorPair.second;
            pAllocator->ReleaseAllocations();

            this->m_availableAllocators.pop();
        }
    }

    if (!pAllocator)
    {
        this->m_allocatorPool.emplace_back(std::make_unique<DynamicSuballocator>(
            this->m_gpuDescriptorHeap,
            this->m_chunkSize));
        pAllocator = this->m_allocatorPool.back().get();
    }

    return pAllocator;
}

void PhxEngine::RHI::Dx12::DynamicSubAllocatorPool::DiscardAllocator(uint64_t fence, DynamicSuballocator* allocator)
{
    this->m_availableAllocators.push(std::make_pair(fence, allocator));
}
