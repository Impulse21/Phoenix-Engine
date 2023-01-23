#pragma once

#include "D3D12Common.h"
#include "D3D12CommandQueue.h"

namespace PhxEngine::RHI::D3D12
{
    class D3D12CommandQueue;
    class D3D12Device;

    struct D3D12DeviceBasicInfo
    {
        uint32_t NumDeviceNodes;
    };

    struct D3D12AdapterDesc
    {
        std::string Name;
        D3D12DeviceBasicInfo DeviceBasicInfo;
        DXGI_ADAPTER_DESC DXGIDesc{};
    };

	class D3D12Adapter
    {
    private:
        static HRESULT EnumAdapters(uint32_t adapterIndex, IDXGIFactory6* factory6, IDXGIAdapter1** outAdapter);

    public:
        D3D12Adapter(D3D12AdapterDesc const& descIn, RefCountPtr<IDXGIAdapter1>& dxgiAdapter);
        ~D3D12Adapter() = default;

        void InitializeD3D12Devices();

    public:
        ID3D12Device* GetRootDevice() { return this->m_rootDevice; }
        ID3D12Device2* GetRootDevice2() { return this->m_rootDevice2; }
        ID3D12Device5* GetRootDevice5() { return this->m_rootDevice5; }
        IDXGIFactory6* GetFactory6() { return this->m_factory6; }

        IDXGIAdapter1* GetDxgiAdapter() { return this->m_dxgiAdapter; }

        const D3D12AdapterDesc& GetDesc() const { return this->m_desc; }

        D3D12Device* GetDevice() const { return this->m_device.get(); }

        const char* GetName() const { return this->GetDesc().Name.c_str(); };
        size_t GetDedicatedSystemMemory() const { return this->GetDesc().DXGIDesc.DedicatedSystemMemory; };
        size_t GetDedicatedVideoMemory() const { return this->GetDesc().DXGIDesc.DedicatedVideoMemory; };
        size_t GetSharedSystemMemory() const { return this->GetDesc().DXGIDesc.SharedSystemMemory; };

    private:
        const D3D12AdapterDesc m_desc;
        RHI::DeviceCapability m_capabilities;

        D3D12_FEATURE_DATA_ROOT_SIGNATURE m_featureDataRootSignature = {};
        D3D12_FEATURE_DATA_SHADER_MODEL   m_featureDataShaderModel = {};
        ShaderModel m_minShaderModel = ShaderModel::SM_6_0;

        // Only support a single device (node)
        std::unique_ptr<D3D12Device> m_device;

        RefCountPtr<ID3D12Device> m_rootDevice;
        RefCountPtr<ID3D12Device2> m_rootDevice2;
        RefCountPtr<ID3D12Device5> m_rootDevice5;

        RefCountPtr<IDXGIFactory6> m_factory6;

        RefCountPtr<IDXGIAdapter1> m_dxgiAdapter;

        // -- Command Queues ---
        std::array<std::unique_ptr<D3D12CommandQueue>, (int)CommandQueueType::Count> m_commandQueues;
    };
}

