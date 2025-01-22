#pragma once

#include "phx/rhi/RHITypes.h"

#ifdef USING_DIRECTX_HEADERS
#include "directx/dxgiformat.h"
#include "directx/d3d12.h"
#include "directx/d3dx12.h"
#include "dxguids/dxguids.h"
#else
#include <d3d12.h>

#include "d3dx12\d3dx12.h"
#endif

#include <dxgi1_6.h>

#define CompPtr Microsoft::WRL::ComPtr

#define SCOPED_LOCK(x) std::scoped_lock _(x)
namespace phx::rhi::dx12
{

    struct NonCopyable
    {
        NonCopyable() = default;
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    };

    struct DxgiFormatMapping
    {
        rhi::Format AbstractFormat;
        DXGI_FORMAT ResourceFormat;
        DXGI_FORMAT SrvFormat;
        DXGI_FORMAT RtvFormat;
    };

    const DxgiFormatMapping& GetDxgiFormatMapping(rhi::Format abstractFormat);

}
