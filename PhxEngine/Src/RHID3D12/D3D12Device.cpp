#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "D3D12Device.h"
#include "D3D12Adapter.h"

// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2

#define ENABLE_PIX_CAPUTRE 1


PhxEngine::RHI::D3D12::D3D12Device::D3D12Device(int32_t nodeMask, D3D12Adapter* parentAdapter)
	: m_parentAdapter(parentAdapter)
	, m_nodeMask(nodeMask)
{
	// Create Queues
	this->m_commandQueues[(int)CommandQueueType::Graphics] = std::make_unique<D3D12CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT, this);
	this->m_commandQueues[(int)CommandQueueType::Compute] = std::make_unique<D3D12CommandQueue>(D3D12_COMMAND_LIST_TYPE_COMPUTE, this);
	this->m_commandQueues[(int)CommandQueueType::Copy] = std::make_unique<D3D12CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY, this);


	// Create Descriptor Heaps
	this->m_cpuDescriptorHeaps[(int)D3D12DescriptorHeapTypes::CBV_SRV_UAV] =
		std::make_unique<D3D12CpuDescriptorHeap>(
			this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	this->m_cpuDescriptorHeaps[(int)D3D12DescriptorHeapTypes::Sampler] =
		std::make_unique<D3D12CpuDescriptorHeap>(
			this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	this->m_cpuDescriptorHeaps[(int)D3D12DescriptorHeapTypes::RTV] =
		std::make_unique<D3D12CpuDescriptorHeap>(
			this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	this->m_cpuDescriptorHeaps[(int)D3D12DescriptorHeapTypes::DSV] =
		std::make_unique<D3D12CpuDescriptorHeap>(
			this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


	this->m_gpuDescriptorHeaps[(int)D3D12DescriptorHeapTypes::CBV_SRV_UAV] =
		std::make_unique<D3D12GpuDescriptorHeap>(
			this,
			NUM_BINDLESS_RESOURCES,
			TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE - NUM_BINDLESS_RESOURCES,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);


	this->m_gpuDescriptorHeaps[(int)D3D12DescriptorHeapTypes::Sampler] =
		std::make_unique<D3D12GpuDescriptorHeap>(
			this,
			10,
			100,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
}

ID3D12Device* PhxEngine::RHI::D3D12::D3D12Device::GetNativeDevice()
{
	return this->m_parentAdapter->GetRootDevice();
}

ID3D12Device2* PhxEngine::RHI::D3D12::D3D12Device::GetNativeDevice2()
{
	return this->m_parentAdapter->GetRootDevice2();
}

ID3D12Device5* PhxEngine::RHI::D3D12::D3D12Device::GetNativeDevice5()
{
	return this->m_parentAdapter->GetRootDevice5();
}

ID3D12CommandAllocator* PhxEngine::RHI::D3D12::D3D12Device::RequestAllocator(RHICommandQueueType queueType)
{
	return nullptr;
}
