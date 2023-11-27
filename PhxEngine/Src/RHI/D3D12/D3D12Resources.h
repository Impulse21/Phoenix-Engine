#pragma once

#include "D3D12Common.h"
#include <D3D12MemAlloc.h>
#include <RHI/D3D12/D3D12DescriptorHeap.h>
namespace PhxEngine::RHI::D3D12
{
    class D3D12GfxDevice;
    class D3D12CommandQueue;

    enum class DescriptorHeapTypes : uint8_t
    {
        CBV_SRV_UAV,
        Sampler,
        RTV,
        DSV,
        Count,
    };

    struct D3D12Shader final : public Core::RefCounter<IShader>
    {
        ShaderDesc Desc = {};
        std::vector<uint8_t> ByteCode;
        Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> RootSignatureDeserializer;
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* RootSignatureDesc = nullptr;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;

        D3D12Shader() = default;
        D3D12Shader(ShaderDesc const& desc, const void* binary, size_t binarySize)
        {
            this->Desc = desc;
            this->ByteCode.resize(binarySize);
            std::memcpy(ByteCode.data(), binary, binarySize);
        }

        ShaderDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<IShader>
    {
        typedef D3D12Shader TConcreteType;
    };

    struct D3D12InputLayout final : public Core::RefCounter<IInputLayout>
    {
        std::vector<VertexAttributeDesc> Attributes;
        std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;

        D3D12InputLayout() = default;

        const Core::Span<VertexAttributeDesc> GetDesc() const override{ return this->Attributes; }
    };

    template<>
    struct TD3D12ResourceTraits<IInputLayout>
    {
        typedef D3D12InputLayout TConcreteType;
    };

    struct D3D12GfxPipeline final : public Core::RefCounter<IGfxPipeline>
    {
        GfxPipelineDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;

        D3D12GfxPipeline() = default;

        GfxPipelineDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<IGfxPipeline>
    {
        typedef D3D12GfxPipeline TConcreteType;
    };

    struct D3D12ComputePipeline final : public Core::RefCounter<IComputePipeline>
    {
        ComputePipelineDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;

        D3D12ComputePipeline() = default;
        ComputePipelineDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<IComputePipeline>
    {
        typedef D3D12ComputePipeline TConcreteType;
    };

    struct D3D12MeshPipeline final : public Core::RefCounter<IMeshPipeline>
    {
        MeshPipelineDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;

        D3D12MeshPipeline() = default;
        MeshPipelineDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<IMeshPipeline>
    {
        typedef D3D12MeshPipeline TConcreteType;
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

    class ICommandSignature
    {};

    struct D3D12CommandSignature final : public Core::RefCounter<ICommandSignature>
    {
        std::vector<D3D12_INDIRECT_ARGUMENT_DESC> D3D12Descs;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> NativeSignature;
    };

    template<>
    struct TD3D12ResourceTraits<ICommandSignature>
    {
        typedef D3D12CommandSignature TConcreteType;
    };

    struct D3D12Texture final : public Core::RefCounter<ITexture>
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

        TextureDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<ITexture>
    {
        typedef D3D12Texture TConcreteType;
    };

    struct D3D12Buffer final : public Core::RefCounter<IBuffer>
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

        BufferDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<IBuffer>
    {
        typedef D3D12Buffer TConcreteType;
    };

#if 0
    struct D3D12RTAccelerationStructure final : public RefCounter<IRTAccelerationStructure>
    {
        // RTAccelerationStructureDesc Desc = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Dx12Desc = {};
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> Geometries;
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info = {};
        // BufferHandle SratchBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;
        DescriptorView Srv;


        void DisposeViews()
        {
            Srv.Allocation.Free();
        }

        D3D12RTAccelerationStructure() = default;
    };

    template<>
    struct TD3D12ResourceTraits<IRTAccelerationStructure>
    {
        typedef D3D12RTAccelerationStructure TConcreteType;
    };
#endif 
#if 0
    struct D3D12TimerQuery final : public Core::RefCounter<Tim>
    {
        size_t BeginQueryIndex = 0;
        size_t EndQueryIndex = 0;

        // Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
        D3D12CommandQueue* CommandQueue = nullptr;
        uint64_t FenceCount = 0;

        bool Started = false;
        bool Resolved = false;
        Core::TimeStep Time;

    };
#endif

    struct D3D12SwapChain final : public Core::RefCounter<ISwapChain>
    {
        SwapChainDesc Desc = {};
        Core::RefCountPtr<IDXGISwapChain1> NativeSwapchain;
        Core::RefCountPtr<IDXGISwapChain4> NativeSwapchain4;

        std::vector<Core::RefCountPtr<ID3D12Resource>> BackBuffers;
        std::vector<DescriptorView> BackBuferViews;

        ~D3D12SwapChain()
        {
            this->DefereDeleteResources();
        }

        void DefereDeleteResources();

        SwapChainDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<ISwapChain>
    {
        typedef D3D12SwapChain TConcreteType;
    };

    struct D3D12DeviceBasicInfo final
    {
        uint32_t NumDeviceNodes;
    };

    class D3D12Adapter final
    {
    public:
        std::string Name;
        size_t DedicatedSystemMemory = 0;
        size_t DedicatedVideoMemory = 0;
        size_t SharedSystemMemory = 0;
        D3D12DeviceBasicInfo BasicDeviceInfo;
        DXGI_ADAPTER_DESC NativeDesc;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> NativeAdapter;

        static HRESULT EnumAdapters(uint32_t adapterIndex, IDXGIFactory6* factory6, IDXGIAdapter1** outAdapter);
    };

}