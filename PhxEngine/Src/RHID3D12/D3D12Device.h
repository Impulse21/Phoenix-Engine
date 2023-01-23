#pragma once

#include<stdint.h>
#include<vector>
#include "D3D12CommandQueue.h"
#include "D3D12DescriptorHeap.h"

namespace PhxEngine::RHI::D3D12
{
	enum class D3D12DescriptorHeapTypes : uint8_t
	{
		CBV_SRV_UAV,
		Sampler,
		RTV,
		DSV,
		Count,
	};

	class D3D12CommandAllocatorPool;
	class D3D12Adapter;
	class D3D12Device
	{
	public:
		D3D12Device(int32_t nodeMask, D3D12Adapter* parentAdapter);

		uint32_t GetNodeMask() const { return this->m_nodeMask; }
		const D3D12Adapter* GetParentAdapter() const { return this->m_parentAdapter; }

		ID3D12Device* GetNativeDevice();
		ID3D12Device2* GetNativeDevice2();
		ID3D12Device5* GetNativeDevice5();

		D3D12CommandQueue* GetQueue(RHICommandQueueType type) { return this->m_commandQueues[(int)type].get(); }

		D3D12CommandQueue* GetGfxQueue() { return this->GetQueue(RHICommandQueueType::Graphics); }
		D3D12CommandQueue* GetComputeQueue() { return this->GetQueue(RHICommandQueueType::Compute); }
		D3D12CommandQueue* GetCopyQueue() { return this->GetQueue(RHICommandQueueType::Copy); }

		ID3D12CommandAllocator* RequestAllocator(RHICommandQueueType queueType);

	private:
		D3D12Adapter* m_parentAdapter;
		uint32_t m_nodeMask = 0;

		// -- Command Queues ---
		std::array<std::unique_ptr<D3D12CommandQueue>, (int)RHICommandQueueType::Count> m_commandQueues;

		// -- Descriptor Heaps ---
		std::array<std::unique_ptr<D3D12CpuDescriptorHeap>, (int)D3D12DescriptorHeapTypes::Count> m_cpuDescriptorHeaps;
		std::array<std::unique_ptr<D3D12GpuDescriptorHeap>, 2> m_gpuDescriptorHeaps;

	};
}

