#pragma once

#include <RHI/RHIEnums.h>
#include <Core/RefCountPtr.h>

#define NOMINMAX
#include <assert.h>

#include "d3d12.h"
#include "d3dx12.h"
#include <dxgi1_6.h>
#ifdef _DEBUG
#include <dxgidebug.h>

#endif

#include "D3D12Adapter.h"
namespace PhxEngine::RHI::D3D12
{
    inline D3D12_RESOURCE_STATES ConvertResourceStates(RHI::ResourceStates stateBits)
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
        if ((stateBits & ResourceStates::StreamOut) != 0) result |= D3D12_RESOURCE_STATE_STREAOUT;
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


    class D3D12Device final
    {
    public:
        D3D12Device(D3D12Adapter const& adapter);
        ~D3D12Device();

        void WaitForIdle();

    public:
        const D3D12Adapter&                         GetGpuAdapter()         const { return this->m_gpuAdapter; }
        Core::RefCountPtr<IDXGIFactory6>            GetNativeFactoryRef()   const { return this->m_dxgiFctory6; }
        Core::RefCountPtr<ID3D12Device>             GetNativeDeviceRef()    const { return this->m_d3d12Device; }
        Core::RefCountPtr<ID3D12Device2>            GetNativeDevice2Ref()   const { return this->m_d3d12Device2; }
        Core::RefCountPtr<ID3D12Device5>            GetNativeDevice5Ref()   const { return this->m_d3d12Device5; }

        IDXGIFactory6*                              GetNativeFactory()  { return this->m_dxgiFctory6.Get(); }
        ID3D12Device*                               GetNativeDevice()   { return this->m_d3d12Device.Get(); }
        ID3D12Device2*                              GetNativeDevice2()  { return this->m_d3d12Device2.Get(); }
        ID3D12Device5*                              GetNativeDevice5()  { return this->m_d3d12Device5.Get(); }

        RHICapability                               GetRhiCapabilities()            const { return this->m_rhiCapabilities; }
        const D3D12_FEATURE_DATA_ROOT_SIGNATURE&    GetFeatureDataRootSignature()   const { return this->m_featureDataRootSignature; }
        const D3D12_FEATURE_DATA_SHADER_MODEL&      GetFeatureDataShaderModel()     const { return this->m_featureDataShaderModel; }
        ShaderModel                                 GetMinShaderModel()             const { return this->m_minShaderModel; }
        bool                                        GetIsUnderGraphicsDebugger()    const { return this->m_isUnderGraphicsDebugger; }

    private:
        D3D12Adapter                         m_gpuAdapter;
        Core::RefCountPtr<IDXGIFactory6>     m_dxgiFctory6;
        Core::RefCountPtr<ID3D12Device>      m_d3d12Device;
        Core::RefCountPtr<ID3D12Device2>     m_d3d12Device2;
        Core::RefCountPtr<ID3D12Device5>     m_d3d12Device5;

        RHICapability                        m_rhiCapabilities;
        D3D12_FEATURE_DATA_ROOT_SIGNATURE    m_featureDataRootSignature;
        D3D12_FEATURE_DATA_SHADER_MODEL      m_featureDataShaderModel;
        ShaderModel                          m_minShaderModel;
        bool                                 m_isUnderGraphicsDebugger;
    };
}
