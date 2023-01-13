#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "D3D12RHI.h"
#include <PhxEngine/RHI/PhxRHI.h>
#include "D3D12Common.h"
#include "D3D12Adapter.h"


using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

namespace
{
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
        if (FAILED(adapter->GetDxgiAdapter()->GetDesc(&adapterDesc)))
        {
            return false;
        }

        // TODO: Check feature level from adapter. Requries device to be created.
        return true;

    }
}

bool D3D12RHIFactory::IsSupported(FeatureLevel requestedFeatureLevel)
{
    /*
    if (!Platform::VerifyWindowsVersion(10, 0, 15063))
    {
        LOG_CORE_WARN("Current windows version doesn't support D3D12. Update to 1703 or newer to for D3D12 support.");
        return false;
    }
    */
    if (!this->m_choosenAdapter)
    {
        this->FindAdapter();
    }

    return this->m_choosenAdapter
        && IsAdapterSupported(this->m_choosenAdapter, requestedFeatureLevel);
}

std::unique_ptr<PhxEngine::RHI::IPhxRHI> PhxEngine::RHI::D3D12::D3D12RHIFactory::CreateRHI()
{
    if (!this->m_choosenAdapter)
    {
        if (!this->IsSupported())
        {
            LOG_CORE_ERROR("Unable to create D3D12 RHI On current platform.");
        }
    }
    
    auto rhi = std::make_unique<D3D12RHI>(this->m_choosenAdapter);
    // Decorate with Validation layer if required

    return rhi;
}

void PhxEngine::RHI::D3D12::D3D12RHIFactory::FindAdapter()
{
    LOG_CORE_INFO("Finding a suitable adapter");

    // Create factory
    RefCountPtr<IDXGIFactory6> factory = this->CreateDXGIFactory6();

    RefCountPtr<IDXGIAdapter1> selectedAdapter;
    size_t selectedGPUVideoMemeory = 0;
    RefCountPtr<IDXGIAdapter1> tempAdapter;
    for (uint32_t adapterIndex = 0; DXGIGpuAdapter::EnumAdapters(adapterIndex, factory.Get(), tempAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
    {
        if (!tempAdapter)
        {
            continue;
        }

        DXGI_ADAPTER_DESC1 desc = {};
        tempAdapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // ignore software adapters
            continue;
        }

        std::string name = NarrowString(desc.Description);
        size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
        size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
        size_t sharedSystemMemory = desc.SharedSystemMemory;

        if (!selectedAdapter || selectedGPUVideoMemeory < dedicatedVideoMemory)
        {
            selectedAdapter = tempAdapter;
            selectedGPUVideoMemeory = dedicatedVideoMemory;
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

    D3D12AdapterDesc adapterDesc =
    {
        .Name = NarrowString(desc.Description),
        .DXGIDesc = desc,
    };
    this->m_choosenAdapter = std::make_shared<D3D12Adapter>(adapterDesc, selectedAdapter);
}

RefCountPtr<IDXGIFactory6> PhxEngine::RHI::D3D12::D3D12RHIFactory::CreateDXGIFactory6() const
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

    RefCountPtr<IDXGIFactory6> factory;
    ThrowIfFailed(
        CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)));

    return factory;
}

D3D12RHI* D3D12RHI::Singleton = nullptr;

D3D12RHI::D3D12RHI(std::shared_ptr<D3D12Adapter>& adapter)
    : m_adapter(adapter)
{
    assert(!Singleton);

    Singleton = this;

    // TODO: lite set up that is required.
}

void PhxEngine::RHI::D3D12::D3D12RHI::Initialize()
{
    this->m_adapter->InitializeD3D12Devices();
    Microsoft::WRL::ComPtr<IUnknown> renderdoc;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, RenderdocUUID, &renderdoc)))
    {
        this->m_isUnderGraphicsDebugger |= !!renderdoc;
    }

    Microsoft::WRL::ComPtr<IUnknown> pix;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, PixUUID, &pix)))
    {
        this->m_isUnderGraphicsDebugger |= !!pix;
    }
}

void PhxEngine::RHI::D3D12::D3D12RHI::Finalize()
{
}
