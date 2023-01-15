#pragma once

#include <locale>
#include <iostream>
#include <string>
#include <sstream>

// #include "dxcapi.h"

#ifdef _DEBUG
#include <dxgidebug.h>

#endif

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d12.lib")


// D3D12
#include "d3d12.h"
#include "d3dx12.h"
#include "d3d12sdklayers.h"
#include "d3d12shader.h"
#include <dxgi1_6.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include "PhxEngine/RHI/RefCountPtr.h"


namespace PhxEngine::RHI::D3D12
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::exception();
        }
    }

    inline std::string NarrowString(const wchar_t* WideStr)
    {
        std::string NarrowStr;

        const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(std::wstringstream().getloc());
        for (auto CurrWChar = WideStr; *CurrWChar != 0; ++CurrWChar)
            NarrowStr.push_back(ctfacet.narrow(*CurrWChar, 0));

        return NarrowStr;
    }

    struct DxgiFormatMapping
    {
        RHIFormat AbstractFormat;
        DXGI_FORMAT ResourcFormatType;
        DXGI_FORMAT SrvFormat;
        DXGI_FORMAT RtvFormat;
    };

   const DxgiFormatMapping& GetDxgiFormatMapping(RHIFormat abstractFormat);

   const DxgiFormatMapping& GetDxgiFormatMapping(FormatType abstractFormat);

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

       return result;
   }

   class D3D12Adapter;
   class D3D12AdapterChild
   {
   public:
       D3D12AdapterChild(D3D12Adapter* parent = nullptr) : ParentAdapter(parent) {}
       virtual ~D3D12AdapterChild() = default;

       FORCEINLINE D3D12Adapter* GetParentAdapter() const
       {
           assert(this->ParentAdapter != nullptr);
           return this->ParentAdapter;
       }

   protected:
       D3D12Adapter* ParentAdapter;

   };
}