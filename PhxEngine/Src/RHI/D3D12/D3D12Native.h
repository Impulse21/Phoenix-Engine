#pragma once

#include "D3D12Common.h"

namespace PhxEngine::RHI::D3D12
{
    Microsoft::WRL::ComPtr<IDXGIFactory6> CreateDXGIFactory6()
    {
        uint32_t flags = 0;
        bool isDebugEnabled= IsDebuggerPresent();

        if (isDebugEnabled)
        {
            Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
            ThrowIfFailed(
                D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

            debugController->EnableDebugLayer();
            flags = DXGI_CREATE_FACTORY_DEBUG;

            Microsoft::WRL::ComPtr<ID3D12Debug3> debugController3;
            if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController3))))
            {
                debugController3->SetEnableGPUBasedValidation(false);
            }

            Microsoft::WRL::ComPtr<ID3D12Debug5> debugController5;
            if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController5))))
            {
                debugController5->SetEnableAutoName(true);
            }
        }

        Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
        ThrowIfFailed(
            CreateDXGIFactory2(flags, IID_PPV_ARGS(factory.ReleaseAndGetAddressOf())));

        return factory;
    }

    bool SafeTestD3D12CreateDevice(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL minFeatureLevel, D3D12DeviceBasicInfo& outInfo)
    {
#pragma warning(disable:6322)
        try
        {
            ID3D12Device* Device = nullptr;
            const HRESULT d3D12CreateDeviceResult = D3D12CreateDevice(adapter, minFeatureLevel, IID_PPV_ARGS(&Device));
            if (SUCCEEDED(d3D12CreateDeviceResult))
            {
                outInfo.NumDeviceNodes = Device->GetNodeCount();

                Device->Release();
                return true;
            }
            else
            {
                LOG_RHI_WARN("D3D12CreateDevice failed.");
            }
        }
        catch (...)
        {
        }
#pragma warning(default:6322)

        return false;
    }

    HRESULT EnumAdapters(uint32_t adapterIndex, IDXGIFactory6* factory6, IDXGIAdapter1** outAdapter)
    {
        return factory6->EnumAdapterByGpuPreference(
            adapterIndex,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(outAdapter));
    }

    void FindAdapter(Microsoft::WRL::ComPtr<IDXGIFactory6> factory, D3D12Adapter& outAdapter)
    {
        LOG_RHI_INFO("Finding a suitable adapter");

        // Create factory
        Microsoft::WRL::ComPtr<IDXGIAdapter1> selectedAdapter;
        D3D12DeviceBasicInfo selectedBasicDeviceInfo = {};
        size_t selectedGPUVideoMemeory = 0;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> tempAdapter;
        for (uint32_t adapterIndex = 0; EnumAdapters(adapterIndex, factory.Get(), tempAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
        {
            if (!tempAdapter)
            {
                continue;
            }

            DXGI_ADAPTER_DESC1 desc = {};
            tempAdapter->GetDesc1(&desc);

            std::string name = NarrowString(desc.Description);
            size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
            size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
            size_t sharedSystemMemory = desc.SharedSystemMemory;

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                LOG_RHI_INFO("GPU '{0}' is a software adapter. Skipping consideration as this is not supported.", name.c_str());
                continue;
            }

            D3D12DeviceBasicInfo basicDeviceInfo = {};
            if (!SafeTestD3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_11_1, basicDeviceInfo))
            {
                continue;
            }

            if (basicDeviceInfo.NumDeviceNodes > 1)
            {
                LOG_RHI_INFO("GPU '{0}' has one or more device nodes. Currently only support one device ndoe.", name.c_str());
            }

            if (!selectedAdapter || selectedGPUVideoMemeory < dedicatedVideoMemory)
            {
                selectedAdapter = tempAdapter;
                selectedGPUVideoMemeory = dedicatedVideoMemory;
                selectedBasicDeviceInfo = basicDeviceInfo;
            }
        }

        if (!selectedAdapter)
        {
            LOG_RHI_WARN("No suitable adapters were found.");
            return;
        }

        DXGI_ADAPTER_DESC desc = {};
        selectedAdapter->GetDesc(&desc);

        std::string name = NarrowString(desc.Description);
        size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
        size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
        size_t sharedSystemMemory = desc.SharedSystemMemory;

        LOG_RHI_INFO(
            "Found Suitable D3D12 Adapter {0}",
            name.c_str());

        LOG_RHI_INFO(
            "Adapter has {0}MB of dedicated video memory, {1}MB of dedicated system memory, and {2}MB of shared system memory.",
            dedicatedVideoMemory / (1024 * 1024),
            dedicatedSystemMemory / (1024 * 1024),
            sharedSystemMemory / (1024 * 1024));

        outAdapter.Name = NarrowString(desc.Description);
        outAdapter.BasicDeviceInfo = selectedBasicDeviceInfo;
        outAdapter.NativeDesc = desc;
        outAdapter.NativeAdapter = selectedAdapter;
    };
}