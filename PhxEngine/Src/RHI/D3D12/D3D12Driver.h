#pragma once

#include <PhxEngine/Core/Log.h>
#include "../GfxDriver.h"

#include "D3D12Common.h"
#include "D3D12CommandQueue.h"

namespace PhxEngine::RHI::D3D12
{
	class D3D12Driver : public GfxDriver
	{
	public:
		inline static D3D12Driver* GPtr;

	public:
		D3D12Driver()
		{
			assert(!GPtr);
			GPtr = this;
		};

		~D3D12Driver()
		{
			assert(GPtr);
			GPtr = nullptr;
		}

		void Initialize() override;
		void Finialize() override;
		void WaitForIdle() override;

		// Interface
	public:
		bool CreateSwapChain(SwapChainDesc const& desc, void* windowHandle, SwapChain& swapchain);

		// Getters
	public:
		Microsoft::WRL::ComPtr<ID3D12Device> GetD3D12Device() { return this->m_rootDevice; }
		Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device2() { return this->m_rootDevice2; }
		Microsoft::WRL::ComPtr<ID3D12Device5> GetD3D12Device5() { return this->m_rootDevice5; }

		Microsoft::WRL::ComPtr<IDXGIFactory6> GetDxgiFactory() { return this->m_factory; }
		Microsoft::WRL::ComPtr<IDXGIAdapter> GetDxgiAdapter() { return this->m_gpuDevice.NativeAdapter; }


		D3D12CommandQueue& GetGfxQueue() { return this->m_commandQueues[CommandQueueType::Graphics]; }
		D3D12CommandQueue& GetComputeQueue() { return this->m_commandQueues[CommandQueueType::Compute]; }
		D3D12CommandQueue& GetCopyQueue() { return this->m_commandQueues[CommandQueueType::Copy]; }

	private:
		void InitializeNativeD3D12Resources();

	private:
		Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
		Microsoft::WRL::ComPtr<ID3D12Device> m_rootDevice;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_rootDevice2;
		Microsoft::WRL::ComPtr<ID3D12Device5> m_rootDevice5;

		D3D12Adapter m_gpuDevice;
		D3D12_FEATURE_DATA_ROOT_SIGNATURE m_featureDataRootSignature = {};
		D3D12_FEATURE_DATA_SHADER_MODEL   m_featureDataShaderModel = {};
		ShaderModel m_minShaderModel = ShaderModel::SM_6_0;

		bool m_isUnderGraphicsDebugger = false;
		RHI::Capability m_capabilities;

		D3D12CommandQueue m_commandQueues[CommandQueueType::Count];
	};
}
