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


// D3D12
#include "d3d12.h"
#include <d3dx12/d3dx12.h>
#include "d3d12sdklayers.h"
#include "d3d12shader.h"
#include <dxgi1_6.h>

#include "RHi/phxRHI.h"
#include "Core/phxEnumClassFlags.h"

namespace phx::rhi::d3d12
{
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
        rhi::Format AbstractFormat;
        DXGI_FORMAT ResourceFormat;
        DXGI_FORMAT SrvFormat;
        DXGI_FORMAT RtvFormat;
    };

   const DxgiFormatMapping& GetDxgiFormatMapping(rhi::Format abstractFormat);

   const DxgiFormatMapping& GetDxgiFormatMapping(rhi::Format abstractFormat);

   inline D3D12_RESOURCE_STATES ConvertResourceStates(ResourceStates stateBits)
   {
       if (stateBits == ResourceStates::Common)
           return D3D12_RESOURCE_STATE_COMMON;

       D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON; // also 0

       if (EnumHasAnyFlags(stateBits, ResourceStates::ConstantBuffer)) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
       if (EnumHasAnyFlags(stateBits, ResourceStates::VertexBuffer)) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
       if (EnumHasAnyFlags(stateBits, ResourceStates::IndexGpuBuffer)) result |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
       if (EnumHasAnyFlags(stateBits, ResourceStates::IndirectArgument)) result |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
       if (EnumHasAnyFlags(stateBits, ResourceStates::ShaderResource)) result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
       if (EnumHasAnyFlags(stateBits, ResourceStates::UnorderedAccess)) result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
       if (EnumHasAnyFlags(stateBits, ResourceStates::RenderTarget)) result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
       if (EnumHasAnyFlags(stateBits, ResourceStates::DepthWrite)) result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
       if (EnumHasAnyFlags(stateBits, ResourceStates::DepthRead)) result |= D3D12_RESOURCE_STATE_DEPTH_READ;
       if (EnumHasAnyFlags(stateBits, ResourceStates::StreamOut)) result |= D3D12_RESOURCE_STATE_STREAM_OUT;
       if (EnumHasAnyFlags(stateBits, ResourceStates::CopyDest)) result |= D3D12_RESOURCE_STATE_COPY_DEST;
       if (EnumHasAnyFlags(stateBits, ResourceStates::CopySource)) result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
       if (EnumHasAnyFlags(stateBits, ResourceStates::ResolveDest)) result |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
       if (EnumHasAnyFlags(stateBits, ResourceStates::ResolveSource)) result |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
       if (EnumHasAnyFlags(stateBits, ResourceStates::Present)) result |= D3D12_RESOURCE_STATE_PRESENT;
       if (EnumHasAnyFlags(stateBits, ResourceStates::AccelStructRead)) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
       if (EnumHasAnyFlags(stateBits, ResourceStates::AccelStructWrite)) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
       if (EnumHasAnyFlags(stateBits, ResourceStates::AccelStructBuildInput)) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
       if (EnumHasAnyFlags(stateBits, ResourceStates::AccelStructBuildBlas)) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
       if (EnumHasAnyFlags(stateBits, ResourceStates::ShadingRateSurface)) result |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
       if (EnumHasAnyFlags(stateBits, ResourceStates::GenericRead)) result |= D3D12_RESOURCE_STATE_GENERIC_READ;
       if (EnumHasAnyFlags(stateBits, ResourceStates::ShaderResourceNonPixel)) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

       return result;
   }
}