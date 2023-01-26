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
#include <PhxEngine/RHI/RefCountPtr.h>


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
        DXGI_FORMAT ResourcRHIFormat;
        DXGI_FORMAT SrvFormat;
        DXGI_FORMAT RtvFormat;
    };

   const DxgiFormatMapping& GetDxgiFormatMapping(RHIFormat abstractFormat);

   const DxgiFormatMapping& GetDxgiFormatMapping(RHIFormat abstractFormat);

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

   class D3D12RHI;
   class D3D12RHIChild
   {
   public:
       D3D12RHIChild(D3D12RHI* parent = nullptr) : ParentRHI(parent) {}
       virtual ~D3D12RHIChild() = default;

       FORCEINLINE D3D12RHI* GetParentRHI() const
       {
           assert(this->ParentRHI != nullptr);
           return this->ParentRHI;
       }

   protected:
       D3D12RHI* ParentRHI;

   };
}