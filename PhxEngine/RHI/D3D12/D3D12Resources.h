#pragma once

#include <Core/Memory.h>
#include <D3D12MemAlloc.h>
#include <D3D12Common.h>
#include <Core/TimeStep.h>
#include <Core/NonCopyable.h>
#include "D3D12DescriptorHeap.h"
#include <RHI/DeferedReleaseQueue.h>

namespace PhxEngine::RHI
{
    struct TextureDesc;
}

namespace PhxEngine::RHI::D3D12
{
    struct D3D12Descriptor : Core::NonCopyable
    {
        D3D12Descriptor() = default;
        D3D12Descriptor(D3D12Descriptor&& other) noexcept
            : Allocation(std::move(other.Allocation)) 
        {};

        D3D12DescriptorAllocation Allocation = {};

        D3D12_CPU_DESCRIPTOR_HANDLE GetView() { return this->Allocation.GetCpuHandle(); }
    };

    struct D3D12SwapChain
    {
        Core::RefCountPtr<IDXGISwapChain1> NativeSwapchain;
        Core::RefCountPtr<IDXGISwapChain4> NativeSwapchain4;
        std::vector<ID3D12Resource*> BackBuffers;
        std::vector<D3D12Descriptor> BackBuferViews;

        explicit operator bool() const
        {
            return !!NativeSwapchain;
        }

        ~D3D12SwapChain();
    };

    class D3D12CommandQueue;
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

    struct D3D12GPUResource : Core::NonCopyable
    {
        Core::RefCountPtr<ID3D12Resource> D3D12Resource;
        Core::RefCountPtr<D3D12MA::Allocation> Allocation;

        explicit operator bool() const
        {
            return !!D3D12Resource;
        }

        D3D12GPUResource() = default;
        virtual ~D3D12GPUResource();
        D3D12GPUResource(D3D12GPUResource&& other) noexcept
            : D3D12Resource(std::move(other.D3D12Resource))
            , Allocation(std::move(other.Allocation)) 
        {
        }

        D3D12GPUResource& operator=(D3D12GPUResource&& other)
        {
            this->D3D12Resource = std::move(other.D3D12Resource);
            this->Allocation = std::move(other.Allocation);

            return *this;
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


    struct D3D12Shader
    {
        std::vector<uint8_t> ByteCode;
    };

    struct D3D12InputLayout
    {
        std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;

        D3D12InputLayout() = default;
    };

    struct D3D12GfxPipeline
    {
        Core::RefCountPtr<ID3D12RootSignature> RootSignature;
        Core::RefCountPtr<ID3D12PipelineState> D3D12PipelineState;

        D3D12GfxPipeline() = default;
    };

    struct D3D12ComputePipeline
    {
        Core::RefCountPtr<ID3D12RootSignature> RootSignature;
        Core::RefCountPtr<ID3D12PipelineState> D3D12PipelineState;
        D3D12ComputePipeline() = default;
    };

    struct D3D12MeshPipeline
    {
        Core::RefCountPtr<ID3D12RootSignature> RootSignature;
        Core::RefCountPtr<ID3D12PipelineState> D3D12PipelineState;

        D3D12MeshPipeline() = default;
    };

    struct D3D12CommandSignature final
    {
        std::vector<D3D12_INDIRECT_ARGUMENT_DESC> D3D12Descs;
        Core::RefCountPtr<ID3D12CommandSignature> NativeSignature;
    };

    struct D3D12Texture final : public D3D12GPUResource
    {
        UINT64 TotalSize = 0;
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> Footprints;
        std::vector<UINT64> RowSizesInBytes;
        std::vector<UINT> NumRows;


        bool CreateRenderTargetView(TextureDesc const& desc, D3D12_CPU_DESCRIPTOR_HANDLE& view) const;
        bool CreateRenderTargetView(D3D12_RENDER_TARGET_VIEW_DESC const& d3d12Resc, D3D12_CPU_DESCRIPTOR_HANDLE& view);

        D3D12Texture() = default;

    };

    struct D3D12Buffer final
    {
        Core::RefCountPtr<ID3D12Resource> D3D12Resource;
        Core::RefCountPtr<D3D12MA::Allocation> Allocation;

        void* MappedData = nullptr;
        uint32_t MappedSizeInBytes = 0;

        D3D12Buffer() = default;

    };

    struct D3D12RTAccelerationStructure final
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Dx12Desc = {};
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> Geometries;
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info = {};
        Core::RefCountPtr<ID3D12Resource> D3D12Resource;
        Core::RefCountPtr<D3D12MA::Allocation> Allocation;

        D3D12RTAccelerationStructure() = default;
    };

}

