#pragma once

namespace PhxEngine::RHI::D3D12
{
    struct D3D12DeviceBasicInfo
    {
        uint32_t NumDeviceNodes;
    };

    class D3D12Adapter final
    {
    public:
        std::string Name;
        size_t DedicatedSystemMemory = 0;
        size_t DedicatedVideoMemory = 0;
        size_t SharedSystemMemory = 0;
        D3D12DeviceBasicInfo BasicDeviceInfo;
        DXGI_ADAPTER_DESC NativeDesc;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> NativeAdapter;
    };

	struct D3D12SwapChain
	{
		Microsoft::WRL::ComPtr<IDXGISwapChain4> Native;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> BackBuffers;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> BackbufferRTV;
	};
}