#pragma once

#include <dstorage.h>
#include <D3D12MemAlloc.h>

#include "D3D12Common.h"
#include "D3D12Adapter.h"

#include "D3D12CommandQueue.h"
#include "RHI/RHIEnums.h"
#include "RHI/RHIResources.h"

namespace PhxEngine::RHI::D3D12
{
    namespace Context
    {
        void Initialize(D3D12Adapter const& adapter, size_t bufferCount);
        void Finalize();
        void WaitForIdle();
        GpuMemoryUsage GetMemoryUsage();

        [[nodiscard]] D3D12Adapter                              GpuAdapter();
        [[nodiscard]] Core::RefCountPtr<IDXGIFactory6>          DxgiFctory6();
        [[nodiscard]] Core::RefCountPtr<ID3D12Device>           D3d12Device();
        [[nodiscard]] Core::RefCountPtr<ID3D12Device2>          D3d12Device2();
        [[nodiscard]] Core::RefCountPtr<ID3D12Device5>          D3d12Device5();

        [[nodiscard]] D3D12CommandQueue&                        Queue(RHI::CommandListType type);

        // -- Direct Storage ---
        [[nodiscard]] Core::RefCountPtr<IDStorageFactory>       DStorageFactory();
        [[nodiscard]] Core::RefCountPtr<IDStorageQueue>	        DStorageQueue(DSTORAGE_REQUEST_SOURCE_TYPE type);
        [[nodiscard]] Core::RefCountPtr<ID3D12Fence>            DStorageFence();

        // -- Direct MA ---
        [[nodiscard]] Core::RefCountPtr<D3D12MA::Allocator>     D3d12MemAllocator();

        [[nodiscard]] RHICapability                             RhiCapabilities();
        [[nodiscard]] D3D12_FEATURE_DATA_ROOT_SIGNATURE         FeatureDataRootSignature();
        [[nodiscard]] D3D12_FEATURE_DATA_SHADER_MODEL           FeatureDataShaderModel();
        [[nodiscard]] ShaderModel                               MinShaderModel();
        [[nodiscard]] bool                                      IsUnderGraphicsDebugger();
        [[nodiscard]] bool                                      DebugEnabled();

        size_t MaxFramesInflight();
    }

}

