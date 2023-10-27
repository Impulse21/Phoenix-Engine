#pragma once

#include "D3D12Common.h"

namespace PhxEngine::RHI::D3D12
{
    struct D3D12DeviceBasicInfo final
    {
        uint32_t NumDeviceNodes;
    };
    struct D3D12Adapter final
    {
        std::string Name;
        size_t DedicatedSystemMemory = 0;
        size_t DedicatedVideoMemory = 0;
        size_t SharedSystemMemory = 0;
        D3D12DeviceBasicInfo BasicDeviceInfo;
        DXGI_ADAPTER_DESC NativeDesc;
        Core::RefCountPtr<IDXGIAdapter1> NativeAdapter;
    };

    HRESULT EnumAdapters(uint32_t adapterIndex, IDXGIFactory6* factory6, IDXGIAdapter1** outAdapter);
}
