#pragma once

#include <assert.h>
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

#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/Core/Log.h>
#include "D3D12RHITypes.h"

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
        RHI::Format AbstractFormat;
        DXGI_FORMAT ResourceFormat;
        DXGI_FORMAT SrvFormat;
        DXGI_FORMAT RtvFormat;
    };

   const DxgiFormatMapping& GetDxgiFormatMapping(RHI::Format abstractFormat);

   const DxgiFormatMapping& GetDxgiFormatMapping(RHI::Format abstractFormat);

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

   // A type cast that is safer than static_cast in debug builds, and is a simple static_cast in release builds.
   // Used for downcasting various ISomething* pointers to their implementation classes in the backends.
   template <typename T, typename U>
   T SafeCast(U u)
   {
	   static_assert(!std::is_same<T, U>::value, "Redundant checked_cast");
#ifdef _DEBUG
	   if (!u) return nullptr;
	   T t = dynamic_cast<T>(u);
	   if (!t) assert(!"Invalid type cast");  // NOLINT(clang-diagnostic-string-conversion)
	   return t;
#else
	   return static_cast<T>(u);
#endif
   }
   
}