#pragma once
#include <dxgi.h>
#include <RHI/RHIEnums.h>

namespace PhxEngine::RHI::D3D12
{
    struct DxgiFormatMapping
    {
        RHI::Format AbstractFormat;
        DXGI_FORMAT ResourceFormat;
        DXGI_FORMAT SrvFormat;
        DXGI_FORMAT RtvFormat;
    };
    const DxgiFormatMapping& GetDxgiFormatMapping(RHI::Format abstractFormat);
}
