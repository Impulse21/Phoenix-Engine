#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "D3D12DeviceFactory.h"
#include "D3D12Common.h"

#include "D3D12GraphicsDevice.h"

using namespace PhxEngine::RHI::D3D12;
static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

static bool sDebugEnabled = false;
bool IsAdapterSupported(std::shared_ptr<D3D12Adapter>& adapter, PhxEngine::RHI::FeatureLevel level)
{
    if (!adapter)
    {
        return false;
    }

    DXGI_ADAPTER_DESC adapterDesc;
    if (FAILED(adapter->NativeAdapter->GetDesc(&adapterDesc)))
    {
        return false;
    }

    // TODO: Check feature level from adapter. Requries device to be created.
    return true;
}

static bool SafeTestD3D12CreateDevice(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL minFeatureLevel, D3D12DeviceBasicInfo& outInfo)
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
            LOG_CORE_WARN("D3D12CreateDevice failed.");
        }
    }
    catch (...)
    {
    }
#pragma warning(default:6322)

    return false;
}

bool D3D12GraphicsDeviceFactory::IsSupported(FeatureLevel requestedFeatureLevel)
{
    return true;
}

std::unique_ptr<PhxEngine::RHI::IGraphicsDevice> PhxEngine::RHI::D3D12::D3D12GraphicsDeviceFactory::CreateDevice()
{
    Microsoft::WRL::ComPtr<IDXGIFactory6> factory = this->CreateDXGIFactory6();

    D3D12Adapter adapter = {};
    this->FindAdapter(factory, adapter);

    if (!adapter.NativeAdapter)
    {
        if (!this->IsSupported())
        {
            LOG_CORE_ERROR("Unable to create D3D12 RHI On current platform.");
        }
    }

    auto device = std::make_unique<D3D12GraphicsDevice>(adapter, factory);

    return device;
}

void PhxEngine::RHI::D3D12::D3D12GraphicsDeviceFactory::FindAdapter(Microsoft::WRL::ComPtr<IDXGIFactory6> factory, D3D12Adapter& outAdapter)
{
    LOG_CORE_INFO("Finding a suitable adapter");

    // Create factory
    Microsoft::WRL::ComPtr<IDXGIAdapter1> selectedAdapter;
    D3D12DeviceBasicInfo selectedBasicDeviceInfo = {};
    size_t selectedGPUVideoMemeory = 0;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> tempAdapter;
    for (uint32_t adapterIndex = 0; D3D12Adapter::EnumAdapters(adapterIndex, factory.Get(), tempAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
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
            LOG_CORE_INFO("GPU '{0}' is a software adapter. Skipping consideration as this is not supported.", name.c_str());
            continue;
        }

        D3D12DeviceBasicInfo basicDeviceInfo = {};
        if (!SafeTestD3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_11_1, basicDeviceInfo))
        {
            continue;
        }

        if (basicDeviceInfo.NumDeviceNodes > 1)
        {
            LOG_CORE_INFO("GPU '{0}' has one or more device nodes. Currently only support one device ndoe.", name.c_str());
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
        LOG_CORE_WARN("No suitable adapters were found.");
        return;
    }

    DXGI_ADAPTER_DESC desc = {};
    selectedAdapter->GetDesc(&desc);

    std::string name = NarrowString(desc.Description);
    size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
    size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
    size_t sharedSystemMemory = desc.SharedSystemMemory;

    LOG_CORE_INFO(
        "Found Suitable D3D12 Adapter {0}",
        name.c_str());

    LOG_CORE_INFO(
        "Adapter has {0}MB of dedicated video memory, {1}MB of dedicated system memory, and {2}MB of shared system memory.",
        dedicatedVideoMemory / (1024 * 1024),
        dedicatedSystemMemory / (1024 * 1024),
        sharedSystemMemory / (1024 * 1024));

    outAdapter.Name = NarrowString(desc.Description);
    outAdapter.BasicDeviceInfo = selectedBasicDeviceInfo;
    outAdapter.NativeDesc = desc;
    outAdapter.NativeAdapter = selectedAdapter;
}

Microsoft::WRL::ComPtr<IDXGIFactory6> PhxEngine::RHI::D3D12::D3D12GraphicsDeviceFactory::CreateDXGIFactory6() const
{
    uint32_t flags = 0;
    sDebugEnabled = IsDebuggerPresent();

    if (sDebugEnabled)
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