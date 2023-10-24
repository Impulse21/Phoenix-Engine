#pragma once

#include <Core/Memory.h>
#include <D3D12Context.h>
#include <RHI/RHIResources.h>

namespace PhxEngine::RHI::D3D12
{
	struct D3D12RHIResource
	{
	private:
		std::atomic<unsigned long> m_refCount = 1;

	public:
		unsigned long AddRef()
		{
			return ++m_refCount;
		}

		unsigned long Release()
		{
			unsigned long result = --m_refCount;
			if (result == 0)
			{
				phx_delete(this);
			}
			return result;
		}
	};

    struct D3D12TimerQuery
    {
        size_t BeginQueryIndex = 0;
        size_t EndQueryIndex = 0;

        // Core::RefCountPtr<ID3D12Fence> Fence;
        D3D12CommandQueue* CommandQueue = nullptr;
        uint64_t FenceCount = 0;

        bool Started = false;
        bool Resolved = false;
        Core::TimeStep Time;

    };

    struct D3D12SwapChain
    {
        SwapChainDesc Desc;

        Core::RefCountPtr<IDXGISwapChain1> NativeSwapchain;
        Core::RefCountPtr<IDXGISwapChain4> NativeSwapchain4;

        std::vector<TextureHandle> BackBuffers;
    };

    struct D3D12Shader
    {
        std::vector<uint8_t> ByteCode;
        const RHI::ShaderDesc Desc;
        Core::RefCountPtr<ID3D12VersionedRootSignatureDeserializer> RootSignatureDeserializer;
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* RootSignatureDesc = nullptr;
        Core::RefCountPtr<ID3D12RootSignature> RootSignature;

        D3D12Shader() = default;
        D3D12Shader(ShaderDesc const& desc, const void* binary, size_t binarySize)
            : Desc(desc)
        {
            this->ByteCode.resize(binarySize);
            std::memcpy(ByteCode.data(), binary, binarySize);
        }
    };

    struct D3D12InputLayout
    {
        std::vector<VertexAttributeDesc> Attributes;
        std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;

        D3D12InputLayout() = default;
    };

    struct D3D12GraphicsPipeline
    {
        GfxPipelineDesc Desc;

        Core::RefCountPtr<ID3D12RootSignature> RootSignature;
        Core::RefCountPtr<ID3D12PipelineState> D3D12PipelineState;

        D3D12GraphicsPipeline() = default;
    };

    struct D3D12ComputePipeline
    {
        Core::RefCountPtr<ID3D12RootSignature> RootSignature;
        Core::RefCountPtr<ID3D12PipelineState> D3D12PipelineState;
        ComputePipelineDesc Desc;

        D3D12ComputePipeline() = default;
    };

    struct D3D12MeshPipeline
    {
        Core::RefCountPtr<ID3D12RootSignature> RootSignature;
        Core::RefCountPtr<ID3D12PipelineState> D3D12PipelineState;
        MeshPipelineDesc Desc;

        D3D12MeshPipeline() = default;
    };

    struct DescriptorView
    {
        DescriptorHeapAllocation Allocation;
        DescriptorIndex BindlessIndex = cInvalidDescriptorIndex;
        D3D12_DESCRIPTOR_HEAP_TYPE Type = {};
        union
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
            D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
            D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
            D3D12_SAMPLER_DESC SAMDesc;
            D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
            D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
        };

        uint32_t FirstMip = 0;
        uint32_t MipCount = 0;
        uint32_t FirstSlice = 0;
        uint32_t SliceCount = 0;
    };

    struct D3D12CommandSignature final
    {
        std::vector<D3D12_INDIRECT_ARGUMENT_DESC> D3D12Descs;
        Core::RefCountPtr<ID3D12CommandSignature> NativeSignature;
    };

    struct D3D12Texture final
    {
        TextureDesc Desc = {};
        Core::RefCountPtr<ID3D12Resource> D3D12Resource;
        Core::RefCountPtr<D3D12MA::Allocation> Allocation;

        UINT64 TotalSize = 0;
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> Footprints;
        std::vector<UINT64> RowSizesInBytes;
        std::vector<UINT> NumRows;

        // -- The views ---
        DescriptorView RtvAllocation;
        std::vector<DescriptorView> RtvSubresourcesAlloc = {};

        DescriptorView DsvAllocation;
        std::vector<DescriptorView> DsvSubresourcesAlloc = {};

        DescriptorView Srv;
        std::vector<DescriptorView> SrvSubresourcesAlloc = {};

        DescriptorView UavAllocation;
        std::vector<DescriptorView> UavSubresourcesAlloc = {};

        D3D12Texture() = default;

        void DisposeViews()
        {
            RtvAllocation.Allocation.Free();
            for (auto& view : this->RtvSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            RtvSubresourcesAlloc.clear();
            RtvAllocation = {};

            DsvAllocation.Allocation.Free();
            for (auto& view : this->DsvSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            DsvSubresourcesAlloc.clear();
            DsvAllocation = {};

            Srv.Allocation.Free();
            for (auto& view : this->SrvSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            SrvSubresourcesAlloc.clear();
            Srv = {};

            UavAllocation.Allocation.Free();
            for (auto& view : this->UavSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            UavSubresourcesAlloc.clear();
            UavAllocation = {};
        }
    };

    struct D3D12Buffer final
    {
        BufferDesc Desc = {};
        Core::RefCountPtr<ID3D12Resource> D3D12Resource;
        Core::RefCountPtr<D3D12MA::Allocation> Allocation;

        void* MappedData = nullptr;
        uint32_t MappedSizeInBytes = 0;

        // -- Views ---
        DescriptorView Srv;
        std::vector<DescriptorView> SrvSubresourcesAlloc = {};

        DescriptorView UavAllocation;
        std::vector<DescriptorView> UavSubresourcesAlloc = {};


        D3D12_VERTEX_BUFFER_VIEW VertexView = {};
        D3D12_INDEX_BUFFER_VIEW IndexView = {};


        const BufferDesc& GetDesc() const { return this->Desc; }

        D3D12Buffer() = default;

        void DisposeViews()
        {
            Srv.Allocation.Free();
            for (auto& view : this->SrvSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            SrvSubresourcesAlloc.clear();
            Srv = {};

            UavAllocation.Allocation.Free();
            for (auto& view : this->UavSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            UavSubresourcesAlloc.clear();
            UavAllocation = {};
        }
    };

    struct D3D12RTAccelerationStructure final
    {
        RTAccelerationStructureDesc Desc = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Dx12Desc = {};
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> Geometries;
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info = {};
        BufferHandle SratchBuffer;
        Core::RefCountPtr<ID3D12Resource> D3D12Resource;
        Core::RefCountPtr<D3D12MA::Allocation> Allocation;
        DescriptorView Srv;


        void DisposeViews()
        {
            Srv.Allocation.Free();
        }

        D3D12RTAccelerationStructure() = default;
    };

    struct D3D12RenderPass final
    {
        RenderPassDesc Desc = {};

        D3D12_RENDER_PASS_FLAGS D12RenderFlags = D3D12_RENDER_PASS_FLAG_NONE;

        size_t NumRenderTargets = 0;
        std::array<D3D12_RENDER_PASS_RENDER_TARGET_DESC, kNumConcurrentRenderTargets> RTVs = {};
        D3D12_RENDER_PASS_DEPTH_STENCIL_DESC DSV = {};

        std::vector<D3D12_RESOURCE_BARRIER> BarrierDescBegin;
        std::vector<D3D12_RESOURCE_BARRIER> BarrierDescEnd;

        D3D12RenderPass() = default;
    };

    struct ResourcePools
    {
        Core::Pool<D3D12Texture, Texture> TexturePool;
        Core::Pool<D3D12CommandSignature, CommandSignature> CommandSignaturePool;
        Core::Pool<D3D12Shader, RHI::Shader> ShaderPool;
        Core::Pool<D3D12InputLayout, InputLayout> InputLayoutPool;
        Core::Pool<D3D12Buffer, Buffer> BufferPool;
        Core::Pool<D3D12RTAccelerationStructure, RTAccelerationStructure> RtAccelerationStructurePool;
        Core::Pool<D3D12GraphicsPipeline, GfxPipeline> GfxPipelinePool;
        Core::Pool<D3D12ComputePipeline, ComputePipeline> ComputePipelinePool;
        Core::Pool<D3D12MeshPipeline, MeshPipeline> MeshPipelinePool;
        Core::Pool<D3D12TimerQuery, TimerQuery> TimerQueryPool;

        void Initialize(size_t initializeSize)
        {
            this->TexturePool.Initialize(initializeSize);
            this->CommandSignaturePool.Initialize(initializeSize);
            this->ShaderPool.Initialize(initializeSize);
            this->InputLayoutPool.Initialize(initializeSize);
            this->BufferPool.Initialize(initializeSize);
            this->RtAccelerationStructurePool.Initialize(initializeSize);
            this->GfxPipelinePool.Initialize(initializeSize);
            this->ComputePipelinePool.Initialize(initializeSize);
            this->MeshPipelinePool.Initialize(initializeSize);
            this->TimerQueryPool.Initialize(initializeSize);
        };

        void Finialize()
        {
            this->TexturePool.Finalize();
            this->CommandSignaturePool.Finalize();
            this->ShaderPool.Finalize();
            this->InputLayoutPool.Finalize();
            this->BufferPool.Finalize();
            this->RtAccelerationStructurePool.Finalize();
            this->GfxPipelinePool.Finalize();
            this->ComputePipelinePool.Finalize();
            this->MeshPipelinePool.Finalize();
            this->TimerQueryPool.Finalize();
        }
    }
}

