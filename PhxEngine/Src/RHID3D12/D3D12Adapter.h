#pragma once

#include "D3D12Common.h"

namespace PhxEngine::RHI::D3D12
{

    struct D3D12AdapterDesc
    {
        std::string Name;

        DXGI_ADAPTER_DESC DXGIDesc{};
    };

	class D3D12Adapter final
    {
    public:
        D3D12Adapter(D3D12AdapterDesc const& descIn, RefCountPtr<IDXGIAdapter1>& dxgiAdapter);
        ~D3D12Adapter() = default;

        void InitializeD3D12Devices();


        ID3D12Device* GetD3D12Device() { return this->m_device; }
        ID3D12Device2* GetD3D12Device2() { return this->m_device2; }
        ID3D12Device5* GetD3D12Device5() { return this->m_device5; }

        IDXGIAdapter1* GetDxgiAdapter() { return this->m_dxgiAdapter; }

        const D3D12AdapterDesc& GetDesc() const { return this->m_desc; }

        const char* GetName() const { return this->GetDesc().Name.c_str(); };
        size_t GetDedicatedSystemMemory() const { return this->GetDesc().DXGIDesc.DedicatedSystemMemory; };
        size_t GetDedicatedVideoMemory() const { return this->GetDesc().DXGIDesc.DedicatedVideoMemory; };
        size_t GetSharedSystemMemory() const { return this->GetDesc().DXGIDesc.SharedSystemMemory; };

    private:
        D3D12AdapterDesc m_desc;
        RHI::DeviceCapability m_capabilities;

        D3D12_FEATURE_DATA_ROOT_SIGNATURE m_featureDataRootSignature = {};
        D3D12_FEATURE_DATA_SHADER_MODEL   m_featureDataShaderModel = {};
        ShaderModel m_minShaderModel = ShaderModel::SM_6_0;

        RefCountPtr<ID3D12Device> m_device;
        RefCountPtr<ID3D12Device2> m_device2;
        RefCountPtr<ID3D12Device5> m_device5;

        RefCountPtr<IDXGIFactory6> m_factory;

        RefCountPtr<IDXGIAdapter1> m_dxgiAdapter;


        static HRESULT EnumAdapters(uint32_t adapterIndex, IDXGIFactory6* factory6, IDXGIAdapter1** outAdapter);
    };
}

