#pragma once

#include "pch.h"
#include "phxGfxCommonResources.h"
#include "phxDeferredReleaseQueue.h"
#include <vector>

namespace phx::gfx
{
	struct D3D12SwapChain : NonCopyable
	{
		Microsoft::WRL::ComPtr<IDXGISwapChain1> m_platform;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> m_platform4;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_backBuffers;

		IDXGISwapChain1* GetPlatform() { return this->m_platform.Get(); }
		IDXGISwapChain1* GetPlatform4() { return this->m_platform4.Get(); }
	};

	using PlatformSwapChain = D3D12SwapChain;



    struct D3D12SwapChain final
    {
        SwapChainDesc Desc;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> NativeSwapchain;
        Microsoft::WRL::ComPtr<IDXGISwapChain4> NativeSwapchain4;

        std::vector<TextureHandle> BackBuffers;
    };

    struct D3D12Shader final
    {
        std::vector<uint8_t> ByteCode;
        const ShaderDesc Desc;
        Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> RootSignatureDeserializer;
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* RootSignatureDesc = nullptr;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;

        D3D12Shader() = default;
        D3D12Shader(ShaderDesc const& desc, const void* binary, size_t binarySize)
            : Desc(desc)
        {
            this->ByteCode.resize(binarySize);
            std::memcpy(ByteCode.data(), binary, binarySize);
        }
    };

    struct D3D12InputLayout final
    {
        std::vector<VertexAttributeDesc> Attributes;
        std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;

        D3D12InputLayout() = default;
    };

    struct D3D12GraphicsPipeline final
    {
        GfxPipelineDesc Desc;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;

        D3D12GraphicsPipeline() = default;
    };

    struct D3D12ComputePipeline final
    {
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
        ComputePipelineDesc Desc;

        D3D12ComputePipeline() = default;
    };

    struct D3D12MeshPipeline final
    {
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
        MeshPipelineDesc Desc;

        D3D12MeshPipeline() = default;
    };

    struct DescriptorView final
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
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> NativeSignature;
    };

    struct D3D12Texture final
    {
        TextureDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;

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
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;

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
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;
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
}