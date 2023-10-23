#pragma once

#include <Core/RefCountPtr.h>

#define NOMINMAX
#include <assert.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#ifdef _DEBUG
#pragma comment(lib, "dxguid.lib")
#endif


#include "d3d12.h"
#include "d3dx12.h"
#include <dxgi1_6.h>
#ifdef _DEBUG
#include <dxgidebug.h>

#endif

#include <Core/Log.h>

#include <deque>
#include "D3D12Adapter.h"
#include "DxgiFormatMapping.h"
#include "Core/SpinLock.h"
namespace PhxEngine::RHI::D3D12
{
    inline D3D12_RESOURCE_STATES ConvertResourceStates(ResourceStates stateBits)
    {
        if (stateBits == ResourceStates::Common)
            return D3D12_RESOURCE_STATE_COMMON;

        D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON; // also 0

        if ((stateBits & ResourceStates::ConstantBuffer) != 0) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        if ((stateBits & ResourceStates::VertexBuffer) != 0) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        if ((stateBits & ResourceStates::IndexGpuBuffer) != 0) result |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
        if ((stateBits & ResourceStates::IndirectArgument) != 0) result |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        if ((stateBits & ResourceStates::ShaderResource) != 0) result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if ((stateBits & ResourceStates::UnorderedAccess) != 0) result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        if ((stateBits & ResourceStates::RenderTarget) != 0) result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
        if ((stateBits & ResourceStates::DepthWrite) != 0) result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
        if ((stateBits & ResourceStates::DepthRead) != 0) result |= D3D12_RESOURCE_STATE_DEPTH_READ;
        if ((stateBits & ResourceStates::StreamOut) != 0) result |= D3D12_RESOURCE_STATE_STREAM_OUT;
        if ((stateBits & ResourceStates::CopyDest) != 0) result |= D3D12_RESOURCE_STATE_COPY_DEST;
        if ((stateBits & ResourceStates::CopySource) != 0) result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
        if ((stateBits & ResourceStates::ResolveDest) != 0) result |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
        if ((stateBits & ResourceStates::ResolveSource) != 0) result |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        if ((stateBits & ResourceStates::Present) != 0) result |= D3D12_RESOURCE_STATE_PRESENT;
        if ((stateBits & ResourceStates::AccelStructRead) != 0) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        if ((stateBits & ResourceStates::AccelStructWrite) != 0) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        if ((stateBits & ResourceStates::AccelStructBuildInput) != 0) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if ((stateBits & ResourceStates::AccelStructBuildBlas) != 0) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        if ((stateBits & ResourceStates::ShadingRateSurface) != 0) result |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
        if ((stateBits & ResourceStates::GenericRead) != 0) result |= D3D12_RESOURCE_STATE_GENERIC_READ;
        if ((stateBits & ResourceStates::ShaderResourceNonPixel) != 0) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        return result;
    }


    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::exception();
        }
    }

    struct DeleteItem
    {
        uint64_t Frame;
        std::function<void()> DeleteFn;
    };


    struct DeleteQueue
    {
        Core::SpinLock Lock;
        std::deque<DeleteItem> Queue;
    };

    class D3D12Context
    {
    public:
        D3D12Context(D3D12Adapter const& gpuAdapter);
        ~D3D12Context();

    private:
        const D3D12Adapter m_gpuAdapter;

        inline static D3D12Context* Singleton; 
        Core::RefCountPtr<IDXGIFactory6> m_dxgiFctory6;
        Core::RefCountPtr<ID3D12Device>  m_d3d12Device;
        Core::RefCountPtr<ID3D12Device2> m_d3d12Device2;
        Core::RefCountPtr<ID3D12Device5> m_d3d12Device5;

        RHICapability m_rhiCapabilities;
        D3D12_FEATURE_DATA_ROOT_SIGNATURE m_featureDataRootSignature;
        D3D12_FEATURE_DATA_SHADER_MODEL   m_featureDataShaderModel; 
        ShaderModel m_minShaderModel;
        DeleteQueue m_deferredDeleteQueue;
        bool m_isUnderGraphicsDebugger;
    };
}

