#include "pch.h"

#include "phxGfxCore_D3D12.h"
#include "D3D12TypeTranslators.h"
#include "Graphics/phxGfxDeviceNotify.h"
#include "Graphics/phxGfxDescriptors.h"

using namespace Microsoft::WRL;
using namespace phx::gfx;
using namespace phx::gfx::d3d12;

using Context = D3D12Context;

#ifdef USING_D3D12_AGILITY_SDK
extern "C"
{
    // Used to enable the "Agility SDK" components
    __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif

namespace
{
    static constexpr size_t MAX_BACK_BUFFER_COUNT = 3;
    static constexpr D3D_FEATURE_LEVEL MIN_FEATURE_LEVEL = D3D_FEATURE_LEVEL_11_1;

    static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
    static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

    inline DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt) noexcept
    {
        switch (fmt)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
        default:                                return fmt;
        }
    }

    inline long ComputeIntersectionArea(
        long ax1, long ay1, long ax2, long ay2,
        long bx1, long by1, long bx2, long by2) noexcept
    {
        return std::max(0l, std::min(ax2, bx2) - std::max(ax1, bx1)) * std::max(0l, std::min(ay2, by2) - std::max(ay1, by1));
    }

    void UpdateColorSpace()
    {
        if (!Context::DxgiFactory)
            return;

        if (!Context::DxgiFactory->IsCurrent())
        {
            // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
            ThrowIfFailed(CreateDXGIFactory2(Context::DxgiFactoryFlags, IID_PPV_ARGS(Context::DxgiFactory.ReleaseAndGetAddressOf())));
        }

        DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

        bool isDisplayHDR10 = false;

        if (Context::SwapChain)
        {
            // To detect HDR support, we will need to check the color space in the primary
            // DXGI output associated with the app at this point in time
            // (using window/display intersection).

            // Get the retangle bounds of the app window.
            RECT windowBounds;
            if (!GetWindowRect(Context::Window, &windowBounds))
                throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "GetWindowRect");

            const long ax1 = windowBounds.left;
            const long ay1 = windowBounds.top;
            const long ax2 = windowBounds.right;
            const long ay2 = windowBounds.bottom;

            ComPtr<IDXGIOutput> bestOutput;
            long bestIntersectArea = -1;

            ComPtr<IDXGIAdapter> adapter;
            for (UINT adapterIndex = 0;
                SUCCEEDED(Context::DxgiFactory->EnumAdapters(adapterIndex, adapter.ReleaseAndGetAddressOf()));
                ++adapterIndex)
            {
                ComPtr<IDXGIOutput> output;
                for (UINT outputIndex = 0;
                    SUCCEEDED(adapter->EnumOutputs(outputIndex, output.ReleaseAndGetAddressOf()));
                    ++outputIndex)
                {
                    // Get the rectangle bounds of current output.
                    DXGI_OUTPUT_DESC desc;
                    ThrowIfFailed(output->GetDesc(&desc));
                    const auto& r = desc.DesktopCoordinates;

                    // Compute the intersection
                    const long intersectArea = ComputeIntersectionArea(ax1, ay1, ax2, ay2, r.left, r.top, r.right, r.bottom);
                    if (intersectArea > bestIntersectArea)
                    {
                        bestOutput.Swap(output);
                        bestIntersectArea = intersectArea;
                    }
                }
            }

            if (bestOutput)
            {
                ComPtr<IDXGIOutput6> output6;
                if (SUCCEEDED(bestOutput.As(&output6)))
                {
                    DXGI_OUTPUT_DESC1 desc;
                    ThrowIfFailed(output6->GetDesc1(&desc));

                    if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
                    {
                        // Display output is HDR10.
                        isDisplayHDR10 = true;
                    }
                }
            }
        }

        if (Context::Desc.EnableHdr && isDisplayHDR10)
        {
            switch (Context::BackBufferFormat)
            {
            case DXGI_FORMAT_R10G10B10A2_UNORM:
                // The application creates the HDR10 signal.
                colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
                break;

            case DXGI_FORMAT_R16G16B16A16_FLOAT:
                // The system creates the HDR10 signal; application uses linear values.
                colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
                break;

            default:
                break;
            }
        }

        Context::ColorSpace = colorSpace;

        UINT colorSpaceSupport = 0;
        if (Context::SwapChain
            && SUCCEEDED(Context::SwapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport))
            && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
        {
            ThrowIfFailed(Context::SwapChain->SetColorSpace1(colorSpace));
        }
    }

    void CreateDeviceResources();
    void CreateWindowDependentResources();
    // Recreate all device resources and set them back to the current state.
    void HandleDeviceLost()
    {
        if (Context::DeviceNotify)
        {
            Context::DeviceNotify->OnDeviceLost();
        }

        for (UINT n = 0; n < Context::BackBufferCount; n++)
        {
            Context::CommandAllocators[n].Reset();
            Context::RenderTargets[n].Reset();
        }

        Context::DepthStencil.Reset();
        Context::CommandQueue.Reset();
        Context::CommandList.Reset();
        Context::Fence.Reset();
        Context::RtvDescriptorHeap.Reset();
        Context::DsvDescriptorHeap.Reset();
        Context::SwapChain.Reset();
        Context::D3dDevice.Reset();
        Context::DxgiFactory.Reset();

#ifdef _DEBUG
        {
            ComPtr<IDXGIDebug1> dxgiDebug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
            {
                dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            }
        }
#endif

        CreateDeviceResources();
        CreateWindowDependentResources();

        if (Context::DeviceNotify)
        {
            Context::DeviceNotify->OnDeviceRestored();
        }
    }

    // This method acquires the first available hardware adapter that supports Direct3D 12.
    // If no such adapter can be found, try WARP. Otherwise throw an exception.
    void FindAdapter(ComPtr<IDXGIFactory6> factory, IDXGIAdapter1** ppAdapter)
    {
        *ppAdapter = nullptr;
        size_t selectedGPUVideoMemeory = 0;
        ComPtr<IDXGIAdapter1> adapter;
        ComPtr<IDXGIAdapter1> selectedAdapter;
        for (UINT adapterIndex = 0;
            SUCCEEDED(factory->EnumAdapterByGpuPreference(
                adapterIndex,
                DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
            adapterIndex++)
        {
            DXGI_ADAPTER_DESC1 desc;
            ThrowIfFailed(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), MIN_FEATURE_LEVEL, __uuidof(ID3D12Device), nullptr)))
            {
                const size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X, VRAM:%zuMB - %ls\n",
                    adapterIndex, desc.VendorId, desc.DeviceId, desc.DedicatedVideoMemory / (1024 * 1024), desc.Description);
                OutputDebugStringW(buff);
#endif
                if (dedicatedVideoMemory > selectedGPUVideoMemeory)
                {
                    selectedGPUVideoMemeory = dedicatedVideoMemory;
                    selectedAdapter = adapter;
                }
            }
        }

#if !defined(NDEBUG)
        if (!selectedAdapter)
        {
            // Try WARP12 instead
            if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
            {
                throw std::runtime_error("WARP12 not available. Enable the 'Graphics Tools' optional feature");
            }

            OutputDebugStringA("Direct3D Adapter - WARP12\n");
        }
#endif

        if (!selectedAdapter)
        {
            throw std::runtime_error("No Direct3D 12 device found");
        }

        *ppAdapter = selectedAdapter.Detach();
    }


    void CreateDevice()
    {
#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        //
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
            {
                debugController->EnableDebugLayer();
            }
            else
            {
                OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
            }

            ComPtr<ID3D12Debug3> debugController3;
            if (SUCCEEDED(debugController.As(&debugController3)))
            {
                debugController3->SetEnableGPUBasedValidation(false);
            }

            ComPtr<ID3D12Debug5> debugController5;
            if (SUCCEEDED(debugController.As(&debugController5)))
            {
                debugController5->SetEnableAutoName(true);
            }

            ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
            {
                Context::DxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
                filter.DenyList.pIDList = hide;
                dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
            }

        }
#endif

    ThrowIfFailed(
        CreateDXGIFactory2(Context::DxgiFactoryFlags, IID_PPV_ARGS(Context::DxgiFactory.ReleaseAndGetAddressOf())));


    // Determines whether tearing support is available for fullscreen borderless windows.
    if (Context::Desc.AllowTearing)
    {
        BOOL allowTearing = FALSE;
        HRESULT hr = Context::DxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        if (FAILED(hr) || !allowTearing)
        {
            Context::Desc.AllowTearing = false;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
    }

    ComPtr<IDXGIAdapter1> adapter;
    FindAdapter(Context::DxgiFactory, adapter.GetAddressOf());

    auto device = Context::D3dDevice;
    // Create the DX12 API device object.
    HRESULT hr = D3D12CreateDevice(
        adapter.Get(),
        MIN_FEATURE_LEVEL,
        IID_PPV_ARGS(device.ReleaseAndGetAddressOf())
    );
    ThrowIfFailed(hr);

#ifndef NDEBUG
    // Configure debug device (if active).
    ComPtr<ID3D12InfoQueue> d3dInfoQueue;
    if (SUCCEEDED(device.As(&d3dInfoQueue)))
    {
#ifdef _DEBUG
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
        D3D12_MESSAGE_ID hide[] =
        {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            // Workarounds for debug layer issues on hybrid-graphics systems
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
        filter.DenyList.pIDList = hide;
        d3dInfoQueue->AddStorageFilterEntries(&filter);
    }
#endif
    Microsoft::WRL::ComPtr<IUnknown> renderdoc;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, RenderdocUUID, &renderdoc)))
    {
        Context::IsUnderGraphicsDebugger |= !!renderdoc;
    }

    Microsoft::WRL::ComPtr<IUnknown> pix;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, PixUUID, &pix)))
    {
        Context::IsUnderGraphicsDebugger |= !!pix;
    }

    // -- Get device capabilities ---
    DeviceCapabilities& capabilites = Context::DeviceCapabilities;
    D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};
    bool hasOptions = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

    if (hasOptions)
    {
        capabilites.RTVTArrayIndexWithoutGS = featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
    }

    // TODO: Move to acability array
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
    bool hasOptions5 = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

    if (SUCCEEDED(device.As(&Context::D3dDevice5)) && hasOptions5)
    {
        capabilites.RayTracing = featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
        capabilites.RenderPass = featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0;
        capabilites.RayQuery = featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;
    }


    D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
    bool hasOptions6 = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

    if (hasOptions6)
    {
        capabilites.VariableRateShading = featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
    bool hasOptions7 = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

    if (SUCCEEDED(device.As(&Context::D3dDevice2)) && hasOptions7)
    {
        capabilites.CreateNoteZeroed = true;
        capabilites.MeshShading = featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;
    }

    // Determine maximum supported feature level for this device
    static const D3D_FEATURE_LEVEL s_featureLevels[] =
    {
#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
        D3D_FEATURE_LEVEL_12_2,
#endif
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
    {
        static_cast<UINT>(std::size(s_featureLevels)), s_featureLevels, D3D_FEATURE_LEVEL_11_0
    };

    hr = device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
    if (SUCCEEDED(hr))
    {
        Context::D3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
    }
    else
    {
        Context::D3dMinFeatureLevel;
    }

    Context::FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(
        Context::D3dDevice2->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &Context::FeatureDataRootSignature, sizeof(Context::FeatureDataRootSignature))))
    {
        Context::FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Check shader model support
    Context::FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
    if (FAILED(
        Context::D3dDevice2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &Context::FeatureDataShaderModel, sizeof(Context::FeatureDataShaderModel.HighestShaderModel))))
    {
        Context::FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
        Context::MinShaderModel = ShaderModel::SM_6_5;
    }
    }

    void CreateDeviceResources()
    {
        // Create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        ThrowIfFailed(
            Context::D3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS((Context::CommandQueue.ReleaseAndGetAddressOf()))));

        Context::CommandQueue->SetName(L"DeviceResources");

        // Create descriptor heaps for render target views and depth stencil views.
        D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
        rtvDescriptorHeapDesc.NumDescriptors = Context::BackBufferCount;
        rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

        ThrowIfFailed(
            Context::D3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(Context::RtvDescriptorHeap.ReleaseAndGetAddressOf())));

        Context::RtvDescriptorHeap->SetName(L"DeviceResources");

        Context::RtvDescriptorSize = Context::D3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Create a command allocator for each back buffer that will be rendered to.
        for (UINT n = 0; n < Context::BackBufferCount; n++)
        {
            ThrowIfFailed(
                Context::D3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(Context::CommandAllocators[n].ReleaseAndGetAddressOf())));

            wchar_t name[25] = {};
            swprintf_s(name, L"Render target %u", n);
            Context::CommandAllocators[n]->SetName(name);
        }

        // Create a command list for recording graphics commands.
        ThrowIfFailed(
            Context::D3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, Context::CommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(Context::CommandList.ReleaseAndGetAddressOf())));
        ThrowIfFailed(Context::CommandList->Close());

        Context::CommandList->SetName(L"DeviceResources");

        // Create a fence for tracking GPU execution progress.
        ThrowIfFailed(
            Context::D3dDevice->CreateFence(Context::FenceValues[Context::BackBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(Context::Fence.ReleaseAndGetAddressOf())));
        Context::FenceValues[Context::BackBufferIndex]++;

        Context::Fence->SetName(L"DeviceResources");

        Context::FenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
        if (!Context::FenceEvent.IsValid())
        {
            throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
        }
    }

    void CreateWindowDependentResources()
    {
        if (!Context::Desc.Window)
        {
            throw std::logic_error("Call SetWindow with a valid Win32 window handle");
        }

        Context::WaitForGpu();

        for (UINT i = 0; i < Context::BackBufferCount; i++)
        {
            Context::RenderTargets[i].Reset();
            Context::FenceValues[i] = Context::FenceValues[D3D12Context::BackBufferIndex];
        }

        // Determine the render target size in pixels.
        const UINT backBufferWidth = std::max<UINT>(static_cast<UINT>(Context::OutputSize.right - Context::OutputSize.left), 1u);
        const UINT backBufferHeight = std::max<UINT>(static_cast<UINT>(Context::OutputSize.bottom - Context::OutputSize.top), 1u);
        const DXGI_FORMAT backBufferFormat = NoSRGB(Context::BackBufferFormat);


        // If the swap chain already exists, resize it, otherwise create one.
        if (Context::SwapChain)
        {
            // If the swap chain already exists, resize it.
            HRESULT hr = Context::SwapChain->ResizeBuffers(
                Context::Desc.BackBufferCount,
                backBufferWidth,
                backBufferHeight,
                backBufferFormat,
                Context::Desc.AllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
            );

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
#ifdef _DEBUG
                char buff[64] = {};
                sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n",
                    static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? Context::D3dDevice->GetDeviceRemovedReason() : hr));
                OutputDebugStringA(buff);
#endif
                // If the device was removed for any reason, a new device and swap chain will need to be created.
                HandleDeviceLost();

                // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
                // and correctly set up the new device.
                return;
            }
            else
            {
                ThrowIfFailed(hr);
            }
        }
        else
        {
            // Create a descriptor for the swap chain.
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = backBufferWidth;
            swapChainDesc.Height = backBufferHeight;
            swapChainDesc.Format = backBufferFormat;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = Context::Desc.BackBufferCount;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            swapChainDesc.Flags = Context::Desc.AllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
            fsSwapChainDesc.Windowed = TRUE;

            // Create a swap chain for the window.
            ComPtr<IDXGISwapChain1> swapChain;
            ThrowIfFailed(
                Context::DxgiFactory->CreateSwapChainForHwnd(
                    Context::CommandQueue.Get(),
                    Context::Desc.Window,
                    &swapChainDesc,
                    &fsSwapChainDesc,
                    nullptr,
                    swapChain.GetAddressOf()));

            ThrowIfFailed(swapChain.As(&Context::SwapChain));

            // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
            ThrowIfFailed(Context::DxgiFactory->MakeWindowAssociation(Context::Desc.Window, DXGI_MWA_NO_ALT_ENTER));
        }

        // Handle color space settings for HDR
        UpdateColorSpace();


        // Obtain the back buffers for this window which will be the final render targets
        // and create render target views for each of them.
        for (UINT n = 0; n < Context::BackBufferCount; n++)
        {
            ThrowIfFailed(Context::SwapChain->GetBuffer(n, IID_PPV_ARGS(Context::RenderTargets[n].GetAddressOf())));

            wchar_t name[25] = {};
            swprintf_s(name, L"Render target %u", n);
            Context::RenderTargets[n]->SetName(name);

            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = Context::BackBufferFormat;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
                Context::RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                static_cast<INT>(n), Context::RtvDescriptorSize);
            Context::D3dDevice->CreateRenderTargetView(Context::RenderTargets[n].Get(), &rtvDesc, rtvDescriptor);
        }

        // Reset the index to the current back buffer.
        Context::BackBufferIndex = Context::SwapChain->GetCurrentBackBufferIndex();
    }
}

void Context::Initialize(phx::gfx::ContextDesc const& desc)
{
    Context::Desc = desc;

    if (desc.BackBufferCount < 2 || desc.BackBufferCount > MAX_BACK_BUFFER_COUNT)
    {
        throw std::out_of_range("invalid backBufferCount");
    }

#ifdef _DEBUG
    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
    }
#endif

    Context::BackBufferFormat = GetDxgiFormatMapping(D3D12Context::Desc.BackBufferFormat).RtvFormat;
    Context::BackBufferCount = D3D12Context::Desc.BackBufferCount;
    Context::OutputSize = {
        .left = 0,
        .top = 0,
        .right = static_cast<long>(Context::Desc.WindowWidth),
        .bottom = static_cast<long>(Context::Desc.WindowHeight),
    };

    CreateDevice();
    CreateDeviceResources();
    CreateWindowDependentResources();

    if (Context::DeviceNotify)
    {
        Context::DeviceNotify->OnDeviceRestored();
    }
}

bool phx::gfx::d3d12::D3D12Context::ResizeSwapchain(int width, int height)
{
    if (!Context::Window)
        return false;

    RECT newRc;
    newRc.left = newRc.top = 0;
    newRc.right = static_cast<long>(width);
    newRc.bottom = static_cast<long>(height);
    if (newRc.right == Context::OutputSize.right && newRc.bottom == Context::OutputSize.bottom)
    {
        // Handle color space settings for HDR
        UpdateColorSpace();

        return false;
    }

    Context::OutputSize = newRc;
    CreateWindowDependentResources();
    return true;
}

void phx::gfx::d3d12::D3D12Context::RegisterDeviceNotify(IDeviceNotify* deviceNotify)
{
    Context::DeviceNotify = deviceNotify;
}

void phx::gfx::d3d12::D3D12Context::WaitForGpu()
{
}

void phx::gfx::d3d12::D3D12Context::Finalize()
{
}

