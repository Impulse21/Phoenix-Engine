#pragma once

#include <mutex>

#include "RHI/phxRHI.h"
#include "Core/phxHandlePool.h"
#include "Core/phxEnumArray.h"

namespace phx::rhi
{
	struct DescriptorAllocationHanlder;
	struct D3D12CommandQueue;

	class D3D12GfxDevice : public GfxDevice
	{
	public:
		static inline GfxDevice* Ptr = nullptr;
		D3D12GfxDevice(Config const& config);
		~D3D12GfxDevice();

		// -- Getters ---
	public:
		Microsoft::WRL::ComPtr<ID3D12Device>&  GetD3D12Device()  { return this->m_d3dDevice; }
		Microsoft::WRL::ComPtr<ID3D12Device2>& GetD3D12Device2() { return this->m_d3dDevice2; }
		Microsoft::WRL::ComPtr<ID3D12Device5>& GetD3D12Device5() { return this->m_d3dDevice5; }

	public:
		operator ID3D12Device*() const { return this->m_d3dDevice.Get(); }
		operator ID3D12Device2* () const { return this->m_d3dDevice2.Get(); }
		operator ID3D12Device5* () const { return this->m_d3dDevice5.Get(); }

	private:
		void CreateDevice(Config const& config);
		void CreateDeviceResources(Config const& config);

	private:
		D3D12ResourceManager m_resourceManager;

		Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;

	private:
		DeviceCapabilities m_capabilities = {};
		std::shared_ptr<DescriptorAllocator> m_descriptorAllocator;

		// -- Device objects ---
		core::EnumArray<rhi::CommandQueueType, D3D12CommandQueue> m_queues;

		// -- Direct3D objects ---
		Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_d3dDevice2;
		Microsoft::WRL::ComPtr<ID3D12Device5> m_d3dDevice5;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocators[MAX_BACK_BUFFER_COUNT];

		// -- Swap chain objects ---
		Microsoft::WRL::ComPtr<IDXGIFactory6> m_dxgiFactory;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[MAX_BACK_BUFFER_COUNT];
		Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;

		// -- Cached device properties. ---
		HWND m_window = NULL;
		D3D_FEATURE_LEVEL m_d3dFeatureLevel;
		D3D12_FEATURE_DATA_ROOT_SIGNATURE m_featureDataRootSignature = {};
		D3D12_FEATURE_DATA_SHADER_MODEL	m_featureDataShaderModel = {};
		ShaderModel m_minShaderModel = ShaderModel::SM_6_6;
		DWORD m_dxgiFactoryFlags;
		RECT m_outputSize;
		bool m_isUnderGraphicsDebugger = false;
		D3D_FEATURE_LEVEL m_d3dMinFeatureLevel = {};
	};

}