#include "phxpch.h"
#include "CommandList.h"

#include <pix3.h>

#include "D3D12GraphicsDevice.h"
#include "DescriptorHeap.h"
#include "BiindlessDescriptorTable.h"
#include "UploadBuffer.h"

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

namespace
{
    // helper function for texture subresource calculations
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dn705766(v=vs.85).aspx
    uint32_t CalcSubresource(uint32_t mipSlice, uint32_t arraySlice, uint32_t planeSlice, uint32_t mipLevels, uint32_t arraySize)
    {
        return mipSlice + (arraySlice * mipLevels) + (planeSlice * mipLevels * arraySize);
    }
}

PhxEngine::RHI::D3D12::D3D12CommandList::D3D12CommandList(CommandQueue* parentQueue)
	: m_parentQueue(parentQueue)
    , m_graphicsDevice(parentQueue->GetGfxDevice())
    , m_uploadBuffer(std::make_unique<UploadBuffer>(parentQueue->GetGfxDevice()))
    , m_dynamicSubAllocatorPool(*parentQueue->GetGfxDevice().GetResourceGpuHeap(), DynamicChunkSizeSrvUavCbv)
{
}

PhxEngine::RHI::D3D12::D3D12CommandList::~D3D12CommandList()
{
    this->m_d3d12CommandList.Reset();
}

void PhxEngine::RHI::D3D12::D3D12CommandList::Open()
{
	assert(!this->m_activeD3D12CommandAllocator);

    this->m_trackedData = std::make_shared<TrackedResources>();

    this->m_activeD3D12CommandAllocator = this->m_parentQueue->RequestAllocator();

    this->m_activeDynamicSubAllocator = this->m_dynamicSubAllocatorPool.RequestAllocator(this->m_parentQueue->GetLastCompletedFence());
    this->m_activeDynamicSubAllocator->ReleaseAllocations();

    if (this->m_d3d12CommandList)
    {
        this->m_d3d12CommandList->Reset(this->m_activeD3D12CommandAllocator.Get(), nullptr);
    }
    else
    {
        this->m_parentQueue->GetGfxDevice().GetD3D12Device2()->CreateCommandList(
            0,
            this->m_parentQueue->GetType(),
            this->m_activeD3D12CommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&this->m_d3d12CommandList));

        this->m_d3d12CommandList->SetName(L"D3D12CommandList::CommandList");
         ThrowIfFailed(
             this->m_d3d12CommandList.As<ID3D12GraphicsCommandList6>(&this->m_d3d12CommandList6));

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

void PhxEngine::RHI::D3D12::D3D12CommandList::Close()
{
    // Is this needed
    assert(this->m_d3d12CommandList);

    this->m_d3d12CommandList->Close();
}

void PhxEngine::RHI::D3D12::D3D12CommandList::RTBuildAccelerationStructure(RHI::RTAccelerationStructureHandle accelStructure)
{
    D3D12RTAccelerationStructure* dx12AccelStructure = this->m_graphicsDevice.GetRTAccelerationStructurePool().Get(accelStructure);
    assert(dx12AccelStructure);


    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC dx12BuildDesc = {};
    dx12BuildDesc.Inputs = dx12AccelStructure->Dx12Desc;

    this->m_buildGeometries = dx12AccelStructure->Geometries;

    // Create a local copy.
    dx12BuildDesc.Inputs.pGeometryDescs = this->m_buildGeometries.data();

    switch (dx12AccelStructure->Desc.Type)
    {
    case RTAccelerationStructureDesc::Type::BottomLevel:
    {
        for (int i = 0; i < dx12AccelStructure->Desc.ButtomLevel.Geometries.size(); i++)
        {
            auto& geometry = dx12AccelStructure->Desc.ButtomLevel.Geometries[i];
            auto& buildGeometry = this->m_buildGeometries[i];

            if (geometry.Flags & RTAccelerationStructureDesc::BottomLevelDesc::Geometry::kOpaque)
            {
                buildGeometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            }
            if (geometry.Flags & RTAccelerationStructureDesc::BottomLevelDesc::Geometry::kNoduplicateAnyHitInvocation)
            {
                buildGeometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
            }

            if (geometry.Type == RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::Triangles)
            {
                D3D12Buffer* vertexBuffer = this->m_graphicsDevice.GetBufferPool().Get(geometry.Triangles.VertexBuffer);
                assert(vertexBuffer);
                buildGeometry.Triangles.VertexBuffer.StartAddress = vertexBuffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)geometry.Triangles.VertexByteOffset;


                D3D12Buffer* indexBuffer = this->m_graphicsDevice.GetBufferPool().Get(geometry.Triangles.IndexBuffer);
                assert(indexBuffer);
                buildGeometry.Triangles.IndexBuffer = indexBuffer->D3D12Resource->GetGPUVirtualAddress() + 
                    (D3D12_GPU_VIRTUAL_ADDRESS)geometry.Triangles.IndexOffset * (geometry.Triangles.IndexFormat == RHIFormat::R16_UINT ? sizeof(uint16_t) : sizeof(uint32_t));

                if (geometry.Flags & RTAccelerationStructureDesc::BottomLevelDesc::Geometry::kUseTransform)
                {
                    D3D12Buffer* transform3x4Buffer = this->m_graphicsDevice.GetBufferPool().Get(geometry.Triangles.Transform3x4Buffer);
                    assert(transform3x4Buffer);
                    buildGeometry.Triangles.Transform3x4 = transform3x4Buffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)geometry.Triangles.Transform3x4BufferOffset;
                }
            }
            else if (geometry.Type == RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::ProceduralAABB)
            {
                D3D12Buffer* aabbBuffer  = this->m_graphicsDevice.GetBufferPool().Get(geometry.AABBs.AABBBuffer);
                assert(aabbBuffer);
                buildGeometry.AABBs.AABBs.StartAddress = aabbBuffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)geometry.AABBs.Offset;
            }
        }
        break;
    }
    case RTAccelerationStructureDesc::Type::TopLevel:
    {
        D3D12Buffer* instanceBuffer = this->m_graphicsDevice.GetBufferPool().Get(dx12AccelStructure->Desc.TopLevel.InstanceBuffer);
        assert(instanceBuffer);

        dx12BuildDesc.Inputs.InstanceDescs = instanceBuffer->D3D12Resource->GetGPUVirtualAddress() +
            (D3D12_GPU_VIRTUAL_ADDRESS)dx12AccelStructure->Desc.TopLevel.Offset;
        break;
    }
    default:
        return;
    }

    // TODO: handle Update
    /*
    if (src != nullptr)
    {
        desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

        auto src_internal = to_internal(src);
        desc.SourceAccelerationStructureData = src_internal->gpu_address;
    }
    */
    dx12BuildDesc.DestAccelerationStructureData = dx12AccelStructure->D3D12Resource->GetGPUVirtualAddress();


    D3D12Buffer* scratchBuffer = this->m_graphicsDevice.GetBufferPool().Get(dx12AccelStructure->SratchBuffer);
    assert(scratchBuffer);

    dx12BuildDesc.ScratchAccelerationStructureData = scratchBuffer->D3D12Resource->GetGPUVirtualAddress();
    this->m_d3d12CommandList6->BuildRaytracingAccelerationStructure(&dx12BuildDesc, 0, nullptr);
}

ScopedMarker D3D12CommandList::BeginScopedMarker(std::string name)
{
    this->BeginMarker(name);
    return ScopedMarker(this);
}

void D3D12CommandList::BeginMarker(std::string name)
{
    PIXBeginEvent(this->m_d3d12CommandList.Get(), 0, std::wstring(name.begin(), name.end()).c_str());
}

void D3D12CommandList::EndMarker()
{
    PIXEndEvent(this->m_d3d12CommandList.Get());
}

void D3D12CommandList::TransitionBarrier(
    TextureHandle texture,
    ResourceStates beforeState,
    ResourceStates afterState)
{
    D3D12Texture* textureImpl = this->m_graphicsDevice.GetTexturePool().Get(texture);
    assert(textureImpl);

    this->TransitionBarrier(
        textureImpl->D3D12Resource,
        ConvertResourceStates(beforeState),
        ConvertResourceStates(afterState));
}

void PhxEngine::RHI::D3D12::D3D12CommandList::TransitionBarrier(
    BufferHandle buffer,
    ResourceStates beforeState,
    ResourceStates afterState)
{
    D3D12Buffer* bufferImpl = this->m_graphicsDevice.GetBufferPool().Get(buffer);
    assert(bufferImpl);

    this->TransitionBarrier(
        bufferImpl->D3D12Resource,
        ConvertResourceStates(beforeState),
        ConvertResourceStates(afterState));
}

void PhxEngine::RHI::D3D12::D3D12CommandList::TransitionBarriers(Core::Span<GpuBarrier> gpuBarriers)
{
    for (const PhxEngine::RHI::GpuBarrier& gpuBarrier : gpuBarriers)
    {
        if (const GpuBarrier::TextureBarrier* texBarrier = std::get_if<GpuBarrier::TextureBarrier>(&gpuBarrier.Data))
        {
            D3D12Texture* textureImpl = this->m_graphicsDevice.GetTexturePool().Get(texBarrier->Texture);
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = textureImpl->D3D12Resource.Get();
            barrier.Transition.StateBefore = ConvertResourceStates(texBarrier->BeforeState);
            barrier.Transition.StateAfter = ConvertResourceStates(texBarrier->AfterState);
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            if (texBarrier->Mip >= 0 || texBarrier->Slice >= 0)
            {
                barrier.Transition.Subresource = D3D12CalcSubresource(
                    (UINT)std::max(0, texBarrier->Mip),
                    (UINT)std::max(0, texBarrier->Slice),
                    0,
                    textureImpl->Desc.MipLevels,
                    textureImpl->Desc.ArraySize);
            }

            this->m_barrierMemoryPool.push_back(barrier);
        }
        else if (const GpuBarrier::BufferBarrier* bufferBarrier = std::get_if<GpuBarrier::BufferBarrier>(&gpuBarrier.Data))
        {
            Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource = this->m_graphicsDevice.GetBufferPool().Get(bufferBarrier->Buffer)->D3D12Resource;
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                D3D12Resource.Get(),
                ConvertResourceStates(bufferBarrier->BeforeState),
                ConvertResourceStates(bufferBarrier->AfterState));

            this->m_barrierMemoryPool.push_back(barrier);
        }

        else if (const GpuBarrier::MemoryBarrier* memoryBarrier = std::get_if<GpuBarrier::MemoryBarrier>(&gpuBarrier.Data))
        {
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.UAV.pResource = nullptr;

            if (const TextureHandle* texture = std::get_if<TextureHandle>(&memoryBarrier->Resource))
            {
                D3D12Texture* textureImpl = this->m_graphicsDevice.GetTexturePool().Get(*texture);
                if (textureImpl && textureImpl->D3D12Resource != nullptr)
                {
                    barrier.UAV.pResource = textureImpl->D3D12Resource.Get();
                }
            }
            else if (const BufferHandle* buffer = std::get_if<BufferHandle>(&memoryBarrier->Resource))
            {
                D3D12Buffer* bufferImpl = this->m_graphicsDevice.GetBufferPool().Get(*buffer);
                if (bufferImpl && bufferImpl->D3D12Resource != nullptr)
                {
                    barrier.UAV.pResource = bufferImpl->D3D12Resource.Get();
                }
            }

            this->m_barrierMemoryPool.push_back(barrier);
        }
    }

    if (!this->m_barrierMemoryPool.empty())
    {
        // TODO: Batch Barrier
        this->m_d3d12CommandList->ResourceBarrier(
            this->m_barrierMemoryPool.size(),
            this->m_barrierMemoryPool.data());

        this->m_barrierMemoryPool.clear();
    }
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BeginRenderPassBackBuffer()
{
    D3D12Viewport& viewport = *this->m_graphicsDevice.GetActiveViewport();
    if (!viewport.RenderPass.IsValid())
    {
        return;
    }

    this->m_activeRenderTarget = viewport.RenderPass;
    D3D12Texture* backBuffer = this->m_graphicsDevice.GetTexturePool().Get(this->m_graphicsDevice.GetBackBuffer());

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = backBuffer->D3D12Resource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    this->m_d3d12CommandList6->ResourceBarrier(1, &barrier);

    D3D12_RENDER_PASS_RENDER_TARGET_DESC RTV = {};
    RTV.cpuDescriptor = backBuffer->RtvAllocation.Allocation.GetCpuHandle();
    RTV.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
    RTV.BeginningAccess.Clear.ClearValue.Color[0] = viewport.Desc.OptmizedClearValue.Colour.R;
    RTV.BeginningAccess.Clear.ClearValue.Color[1] = viewport.Desc.OptmizedClearValue.Colour.G;
    RTV.BeginningAccess.Clear.ClearValue.Color[2] = viewport.Desc.OptmizedClearValue.Colour.B;
    RTV.BeginningAccess.Clear.ClearValue.Color[3] = viewport.Desc.OptmizedClearValue.Colour.A;
    RTV.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    this->m_d3d12CommandList6->BeginRenderPass(1, &RTV, nullptr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);
}

RenderPassHandle PhxEngine::RHI::D3D12::D3D12CommandList::GetRenderPassBackBuffer()
{
    D3D12Viewport& viewport = *this->m_graphicsDevice.GetActiveViewport();
    return viewport.RenderPass;
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BeginRenderPass(RenderPassHandle renderPass)
{
    D3D12RenderPass* renderPassImpl = this->m_graphicsDevice.GetRenderPassPool().Get(renderPass);
    if (renderPass == this->m_graphicsDevice.GetActiveViewport()->RenderPass)
    {
        D3D12Texture* backBuffer = this->m_graphicsDevice.GetTexturePool().Get(this->m_graphicsDevice.GetBackBuffer());

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = backBuffer->D3D12Resource.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        this->m_d3d12CommandList6->ResourceBarrier(1, &barrier);
    }
    else
    {
        if (!renderPassImpl)
        {
            return;
        }
        // Transiion Barriers
        if (!renderPassImpl->BarrierDescBegin.empty())
        {
            this->m_d3d12CommandList->ResourceBarrier(
                (UINT)renderPassImpl->BarrierDescBegin.size(),
                renderPassImpl->BarrierDescBegin.data());
        }
    }


    this->m_d3d12CommandList6->BeginRenderPass(
        (UINT)renderPassImpl->NumRenderTargets,
        renderPassImpl->RTVs.data(),
        renderPassImpl->DSV.cpuDescriptor.ptr == 0 ? nullptr : &renderPassImpl->DSV,
        renderPassImpl->D12RenderFlags);

    this->m_activeRenderTarget = renderPass;
}

void PhxEngine::RHI::D3D12::D3D12CommandList::EndRenderPass()
{
    if (!this->m_activeRenderTarget.IsValid())
    {
        return;
    }

    this->m_d3d12CommandList6->EndRenderPass();

    if (this->m_activeRenderTarget == this->m_graphicsDevice.GetActiveViewport()->RenderPass)
    {
        D3D12Texture* backBuffer = this->m_graphicsDevice.GetTexturePool().Get(this->m_graphicsDevice.GetBackBuffer());

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = backBuffer->D3D12Resource.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        this->m_d3d12CommandList6->ResourceBarrier(1, &barrier);
    }
    else
    {
        D3D12RenderPass* renderPassImpl = this->m_graphicsDevice.GetRenderPassPool().Get(this->m_activeRenderTarget);
        if (!renderPassImpl)
        {
            return;
        }

        // Transiion Barriers
        if (!renderPassImpl->BarrierDescEnd.empty())
        {
            this->m_d3d12CommandList->ResourceBarrier(
                (UINT)renderPassImpl->BarrierDescEnd.size(),
                renderPassImpl->BarrierDescEnd.data());
        }
    }

    this->m_activeRenderTarget = {};
}

void D3D12CommandList::ClearTextureFloat(TextureHandle texture, Color const& clearColour)
{
    auto textureImpl = this->m_graphicsDevice.GetTexturePool().Get(texture);
    assert(textureImpl);

    assert(!textureImpl->RtvAllocation.Allocation.IsNull());
    this->m_d3d12CommandList->ClearRenderTargetView(
        textureImpl->RtvAllocation.Allocation.GetCpuHandle(),
        &clearColour.R,
        0,
        nullptr);
    this->m_trackedData->TextureHandles.push_back(texture);
}


GPUAllocation D3D12CommandList::AllocateGpu(size_t bufferSize, size_t stride)
{
    assert(bufferSize <= this->m_uploadBuffer->GetPageSize());

    auto heapAllocation = this->m_uploadBuffer->Allocate(bufferSize, stride);

    GPUAllocation gpuAlloc = {};
    gpuAlloc.GpuBuffer = heapAllocation.BufferHandle;
    gpuAlloc.CpuData = heapAllocation.CpuData;
    gpuAlloc.Offset = heapAllocation.Offset;

    return gpuAlloc;
}

void D3D12CommandList::ClearDepthStencilTexture(
    TextureHandle depthStencil,
    bool clearDepth,
    float depth,
    bool clearStencil,
    uint8_t stencil)
{
    auto textureImpl = this->m_graphicsDevice.GetTexturePool().Get(depthStencil);
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

    assert(!textureImpl->DsvAllocation.Allocation.IsNull());
    this->m_d3d12CommandList->ClearDepthStencilView(
        textureImpl->DsvAllocation.Allocation.GetCpuHandle(),
        flags,
        depth,
        stencil,
        0,
        nullptr);
    this->m_trackedData->TextureHandles.push_back(depthStencil);
}

void D3D12CommandList::Draw(
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

void D3D12CommandList::DrawIndexed(
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

constexpr uint32_t AlignTo(uint32_t value, uint32_t alignment)
{
    return ((value + alignment - 1) / alignment) * alignment;
}

constexpr uint64_t AlignTo(uint64_t value, uint64_t alignment)
{
    return ((value + alignment - 1) / alignment) * alignment;
}
void D3D12CommandList::WriteBuffer(BufferHandle buffer, const void* data, size_t dataSize, uint64_t destOffsetBytes)
{
    UINT64 alignedSize = dataSize;
    const D3D12Buffer* bufferImpl = this->m_graphicsDevice.GetBufferPool().Get(buffer);
    if ((bufferImpl->GetDesc().Binding & BindingFlags::ConstantBuffer) == BindingFlags::ConstantBuffer)
    {
        alignedSize = AlignTo(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    }

    auto heapAllocation = this->m_uploadBuffer->Allocate(dataSize, alignedSize);
    D3D12Buffer* uploadBufferImpl = this->m_graphicsDevice.GetBufferPool().Get(heapAllocation.BufferHandle);
    memcpy(heapAllocation.CpuData, data, dataSize);
    this->m_d3d12CommandList->CopyBufferRegion(
        bufferImpl->D3D12Resource.Get(),
        destOffsetBytes,
        uploadBufferImpl->D3D12Resource.Get(),
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
}

void D3D12CommandList::CopyBuffer(BufferHandle dst, uint64_t dstOffset, BufferHandle src, uint64_t srcOffset, size_t sizeInBytes)
{
    const D3D12Buffer* srcBuffer = this->m_graphicsDevice.GetBufferPool().Get(src);
    const D3D12Buffer* dstBuffer = this->m_graphicsDevice.GetBufferPool().Get(dst);

    this->m_d3d12CommandList->CopyBufferRegion(
        dstBuffer->D3D12Resource.Get(),
        (UINT64)dstOffset,
        srcBuffer->D3D12Resource.Get(),
        (UINT64)srcOffset,
        (UINT64)sizeInBytes);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::WriteTexture(TextureHandle texture, uint32_t firstSubresource, size_t numSubresources, SubresourceData* pSubresourceData)
{
    auto textureImpl = this->m_graphicsDevice.GetTexturePool().Get(texture);
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

    this->m_graphicsDevice.DeleteD3DResource(intermediateResource);
    this->m_trackedData->TextureHandles.push_back(texture);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::WriteTexture(TextureHandle texture, uint32_t arraySlice, uint32_t mipLevel, const void* data, size_t rowPitch, size_t depthPitch)
{
    // LOG_CORE_FATAL("NOT IMPLEMENTED FUNCTION CALLED: CommandList::WriteTexture");
    assert(false);
    auto textureImpl = this->m_graphicsDevice.GetTexturePool().Get(texture);
    uint32_t subresource = CalcSubresource(mipLevel, arraySlice, 0, textureImpl->Desc.MipLevels, textureImpl->Desc.ArraySize);

    D3D12_RESOURCE_DESC resourceDesc = textureImpl->D3D12Resource->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    uint32_t numRows;
    uint64_t rowSizeInBytes;
    uint64_t totalBytes;
}

void PhxEngine::RHI::D3D12::D3D12CommandList::SetRenderTargets(std::vector<TextureHandle> const& renderTargets, TextureHandle depthStencil)
{
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetViews(renderTargets.size());
    for (int i = 0; i < renderTargets.size(); i++)
    {
        this->m_trackedData->TextureHandles.push_back(renderTargets[i]);
        auto textureImpl = this->m_graphicsDevice.GetTexturePool().Get(renderTargets[i]);
        renderTargetViews[i] = textureImpl->RtvAllocation.Allocation.GetCpuHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE depthView = {};
    bool hasDepth = depthStencil.IsValid();
    if (hasDepth)
    {
        auto textureImpl = this->m_graphicsDevice.GetTexturePool().Get(depthStencil);
        depthView = textureImpl->DsvAllocation.Allocation.GetCpuHandle();
    }

    this->m_d3d12CommandList->OMSetRenderTargets(
        renderTargetViews.size(),
        renderTargetViews.data(),
        hasDepth,
        hasDepth ? &depthView : nullptr);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::SetGraphicsPipeline(GraphicsPipelineHandle graphicsPiplineHandle)
{
    this->m_activeComputePipeline = nullptr;
    D3D12GraphicsPipeline* graphisPipeline = this->m_graphicsDevice.GetGraphicsPipelinePool().Get(graphicsPiplineHandle);
    this->m_d3d12CommandList->SetPipelineState(graphisPipeline->D3D12PipelineState.Get());

    this->m_d3d12CommandList->SetGraphicsRootSignature(graphisPipeline->RootSignature.Get());

    const auto& desc = graphisPipeline->Desc;
    D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    switch (desc.PrimType)
    {
    case PrimitiveType::TriangleList:
        topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        break;
    case PrimitiveType::TriangleStrip:
        topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        break;
    case PrimitiveType::LineList:
        topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        break;
    default:
        assert(false);
    }
    this->m_d3d12CommandList->IASetPrimitiveTopology(topology);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::SetViewports(Viewport* viewports, size_t numViewports)
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

void PhxEngine::RHI::D3D12::D3D12CommandList::SetScissors(Rect* scissors, size_t numScissors)
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

std::shared_ptr<TrackedResources> D3D12CommandList::Executed(uint64_t fenceValue)
{
    this->m_parentQueue->DiscardAllocator(fenceValue, this->m_activeD3D12CommandAllocator.Get());
    this->m_activeD3D12CommandAllocator = nullptr;

    this->m_dynamicSubAllocatorPool.DiscardAllocator(fenceValue, this->m_activeDynamicSubAllocator);
    this->m_activeDynamicSubAllocator = nullptr;

    auto trackedResources = this->m_trackedData;
    this->m_trackedData.reset();

    return trackedResources;
}

void PhxEngine::RHI::D3D12::D3D12CommandList::SetDescritporHeaps(std::array<GpuDescriptorHeap*, 2> const& shaderHeaps)
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

void D3D12CommandList::TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        d3d12Resource.Get(),
        beforeState, afterState);

    // TODO: Batch Barrier
    this->m_d3d12CommandList->ResourceBarrier(1, &barrier);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants)
{
    if (this->m_activeComputePipeline)
    {
        this->m_d3d12CommandList->SetComputeRoot32BitConstants(rootParameterIndex, sizeInBytes / sizeof(uint32_t), constants, 0);
    }
    else
    {
        this->m_d3d12CommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, sizeInBytes / sizeof(uint32_t), constants, 0);
    }
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindConstantBuffer(size_t rootParameterIndex, BufferHandle constantBuffer)
{
    const D3D12Buffer* constantBufferImpl = this->m_graphicsDevice.GetBufferPool().Get(constantBuffer);
    if (this->m_activeComputePipeline)
    {
        this->m_d3d12CommandList->SetComputeRootConstantBufferView(rootParameterIndex, constantBufferImpl->D3D12Resource->GetGPUVirtualAddress());
    }
    else
    {
        this->m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, constantBufferImpl->D3D12Resource->GetGPUVirtualAddress());
    }
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
    UploadBuffer::Allocation alloc = this->m_uploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    std::memcpy(alloc.CpuData, bufferData, sizeInBytes);

    if (this->m_activeComputePipeline)
    {
        this->m_d3d12CommandList->SetComputeRootConstantBufferView(rootParameterIndex, alloc.Gpu);
    }
    else
    {
        this->m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, alloc.Gpu);
    }
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindVertexBuffer(uint32_t slot, BufferHandle vertexBuffer)
{
    const D3D12Buffer* vertexBufferImpl = this->m_graphicsDevice.GetBufferPool().Get(vertexBuffer);

    this->m_d3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferImpl->VertexView);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
{
    size_t bufferSize = numVertices * vertexSize;
    auto heapAllocation = this->m_uploadBuffer->Allocate(bufferSize, vertexSize);
    memcpy(heapAllocation.CpuData, vertexBufferData, bufferSize);

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = heapAllocation.Gpu;
    vertexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
    vertexBufferView.StrideInBytes = static_cast<UINT>(vertexSize);

    this->m_d3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindIndexBuffer(BufferHandle indexBuffer)
{
    const D3D12Buffer* bufferImpl = this->m_graphicsDevice.GetBufferPool().Get(indexBuffer);

    this->m_d3d12CommandList->IASetIndexBuffer(&bufferImpl->IndexView);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindDynamicIndexBuffer(size_t numIndicies, RHIFormat indexFormat, const void* indexBufferData)
{
    size_t indexSizeInBytes = indexFormat == RHIFormat::R16_UINT ? 2 : 4;
    size_t bufferSize = numIndicies * indexSizeInBytes;

    auto heapAllocation = this->m_uploadBuffer->Allocate(bufferSize, indexSizeInBytes);
    memcpy(heapAllocation.CpuData, indexBufferData, bufferSize);

    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.BufferLocation = heapAllocation.Gpu;
    indexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
    const auto& formatMapping = GetDxgiFormatMapping(indexFormat);;
    
    indexBufferView.Format = formatMapping.SrvFormat;

    this->m_d3d12CommandList->IASetIndexBuffer(&indexBufferView);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData)
{
    size_t sizeInBytes = numElements * elementSize;
    UploadBuffer::Allocation alloc = this->m_uploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    std::memcpy(alloc.CpuData, bufferData, sizeInBytes);

    if (this->m_activeComputePipeline)
    {
        this->m_d3d12CommandList->SetComputeRootShaderResourceView(rootParameterIndex, alloc.Gpu);
    }
    else
    {
        this->m_d3d12CommandList->SetGraphicsRootShaderResourceView(rootParameterIndex, alloc.Gpu);
    }
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindStructuredBuffer(size_t rootParameterIndex, BufferHandle buffer)
{
    const D3D12Buffer* bufferImpl = this->m_graphicsDevice.GetBufferPool().Get(buffer);

    if (this->m_activeComputePipeline)
    {
        this->m_d3d12CommandList->SetComputeRootShaderResourceView(
            rootParameterIndex,
            bufferImpl->D3D12Resource->GetGPUVirtualAddress());
    }
    else
    {
        this->m_d3d12CommandList->SetGraphicsRootShaderResourceView(
            rootParameterIndex,
            bufferImpl->D3D12Resource->GetGPUVirtualAddress());
    }
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindResourceTable(size_t rootParameterIndex)
{
    if (this->m_graphicsDevice.GetMinShaderModel() < ShaderModel::SM_6_6)
    {
        if (this->m_activeComputePipeline)
        {
            this->m_d3d12CommandList->SetComputeRootDescriptorTable(
                rootParameterIndex,
                this->m_graphicsDevice.GetBindlessTable()->GetGpuHandle(0));
        }
        else
        {
            this->m_d3d12CommandList->SetGraphicsRootDescriptorTable(
                rootParameterIndex,
                this->m_graphicsDevice.GetBindlessTable()->GetGpuHandle(0));
        }
    }
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindSamplerTable(size_t rootParameterIndex)
{
    // TODO:
    assert(false);
}

void D3D12CommandList::BindDynamicDescriptorTable(size_t rootParameterIndex, Core::Span<TextureHandle> textures)
{
    // Request Descriptoprs for table
    // Validate with Root Signature. Maybe an improvment in the future.
    DescriptorHeapAllocation descriptorTable = this->m_activeDynamicSubAllocator->Allocate(textures.Size());
    for (int i = 0; i < textures.Size(); i++)
    {
        this->m_trackedData->TextureHandles.push_back(textures[i]);
        auto textureImpl = this->m_graphicsDevice.GetTexturePool().Get(textures[i]);
        this->m_graphicsDevice.GetD3D12Device2()->CopyDescriptorsSimple(
            1,
            descriptorTable.GetCpuHandle(i),
            textureImpl->Srv.Allocation.GetCpuHandle(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    if (this->m_activeComputePipeline)
    {
        this->m_d3d12CommandList->SetComputeRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
    }
    else
    {
        this->m_d3d12CommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
    }
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BindDynamicUavDescriptorTable(size_t rootParameterIndex, std::vector<TextureHandle> const& textures)
{
    // Request Descriptoprs for table
    // Validate with Root Signature. Maybe an improvment in the future.
    DescriptorHeapAllocation descriptorTable = this->m_activeDynamicSubAllocator->Allocate(textures.size());
    for (int i = 0; i < textures.size(); i++)
    {
        this->m_trackedData->TextureHandles.push_back(textures[i]);
        auto textureImpl = this->m_graphicsDevice.GetTexturePool().Get(textures[i]);
        this->m_graphicsDevice.GetD3D12Device2()->CopyDescriptorsSimple(
            1,
            descriptorTable.GetCpuHandle(i),
            textureImpl->UavAllocation.Allocation.GetCpuHandle(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    }

    if (this->m_activeComputePipeline)
    {
        this->m_d3d12CommandList->SetComputeRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
    }
    else
    {
        this->m_d3d12CommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
    }
}

void PhxEngine::RHI::D3D12::D3D12CommandList::SetComputeState(ComputePipelineHandle state)
{
    D3D12ComputePipeline* computePsoImpl = this->m_graphicsDevice.GetComputePipelinePool().Get(state);
    this->m_d3d12CommandList->SetComputeRootSignature(computePsoImpl->RootSignature.Get());
    this->m_d3d12CommandList->SetPipelineState(computePsoImpl->D3D12PipelineState.Get());

    this->m_activeComputePipeline = computePsoImpl;
}

void PhxEngine::RHI::D3D12::D3D12CommandList::Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
{
    this->m_d3d12CommandList->Dispatch(groupsX, groupsY, groupsZ);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::DispatchIndirect(uint32_t offsetBytes)
{
    // Not supported yet
    assert(false);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::BeginTimerQuery(TimerQueryHandle query)
{
    auto queryImpl = std::static_pointer_cast<TimerQuery>(query);

    this->m_trackedData->TimerQueries.push_back(queryImpl);

    this->m_d3d12CommandList->EndQuery(
        this->m_graphicsDevice.GetQueryHeap(),
        D3D12_QUERY_TYPE_TIMESTAMP,
        queryImpl->BeginQueryIndex);
}

void PhxEngine::RHI::D3D12::D3D12CommandList::EndTimerQuery(TimerQueryHandle query)
{
    auto queryImpl = std::static_pointer_cast<TimerQuery>(query);


    const D3D12Buffer* timeStampQueryBuffer = this->m_graphicsDevice.GetBufferPool().Get(this->m_graphicsDevice.GetTimestampQueryBuffer());

    this->m_trackedData->TimerQueries.push_back(queryImpl);

    this->m_d3d12CommandList->EndQuery(
        this->m_graphicsDevice.GetQueryHeap(),
        D3D12_QUERY_TYPE_TIMESTAMP,
        queryImpl->EndQueryIndex);

    this->m_d3d12CommandList->ResolveQueryData(
        this->m_graphicsDevice.GetQueryHeap(),
        D3D12_QUERY_TYPE_TIMESTAMP,
        queryImpl->BeginQueryIndex,
        2,
        timeStampQueryBuffer->D3D12Resource.Get(),
        queryImpl->BeginQueryIndex * sizeof(uint64_t));
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

void PhxEngine::RHI::D3D12::DynamicSubAllocatorPool::DiscardAllocator(uint64_t fence, DynamicSuballocator* allocator)
{
    this->m_availableAllocators.push(std::make_pair(fence, allocator));
}
