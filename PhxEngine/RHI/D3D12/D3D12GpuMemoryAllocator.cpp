#include "D3D12GpuMemoryAllocator.h"

#include "D3D12Device.h"

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

PhxEngine::RHI::D3D12::D3D12GpuMemoryAllocator::D3D12GpuMemoryAllocator(std::shared_ptr<D3D12Device> device)
{
	D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
	allocatorDesc.pDevice = device->GetNativeDevice();
	allocatorDesc.pAdapter = device->GetGpuAdapter().NativeAdapter.Get();
	//allocatorDesc.PreferredBlockSize = 256 * 1024 * 1024;
	//allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
	allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS)(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);

	ThrowIfFailed(
		D3D12MA::CreateAllocator(&allocatorDesc, &this->m_d3d12MemAllocator));
}

void PhxEngine::RHI::D3D12::D3D12GpuMemoryAllocator::AllocateResource(D3D12MA::ALLOCATION_DESC const& allocationDesc, CD3DX12_RESOURCE_DESC const& resourceDesc, D3D12_RESOURCE_STATES initialState, D3D12MA::Allocation** outAllocation, Core::RefCountPtr<ID3D12Resource> d3d12Resource)
{
	ThrowIfFailed(
		this->m_d3d12MemAllocator->CreateResource(
			&allocationDesc,
			&resourceDesc,
			initialState,
			nullptr,
			outAllocation,
			IID_PPV_ARGS(&d3d12Resource)));
}

void PhxEngine::RHI::D3D12::D3D12GpuMemoryAllocator::AllocateAliasingResource(D3D12MA::Allocation* aliasedResourceAllocation, CD3DX12_RESOURCE_DESC const& resourceDesc, D3D12_RESOURCE_STATES initialState, Core::RefCountPtr<ID3D12Resource> d3d12Resource)
{
	ThrowIfFailed(
		this->m_d3d12MemAllocator->CreateAliasingResource(
			aliasedResourceAllocation,
			0,
			&resourceDesc,
			initialState,
			nullptr,
			IID_PPV_ARGS(&d3d12Resource)));
}

void PhxEngine::RHI::D3D12::D3D12GpuMemoryAllocator::AllocateAliasedResource(D3D12MA::ALLOCATION_DESC const& allocationDesc, D3D12_RESOURCE_ALLOCATION_INFO const& finalAllocInfo, D3D12MA::Allocation** outAllocation)
{
	ThrowIfFailed(
		this->m_d3d12MemAllocator->AllocateMemory(
			&allocationDesc,
			&finalAllocInfo,
			outAllocation));
}


RHI::GpuMemoryUsage PhxEngine::RHI::D3D12::D3D12GpuMemoryAllocator::GetMemoryUsage() const
{
    D3D12MA::Budget budget;
    this->m_d3d12MemAllocator->GetBudget(&budget, nullptr);

    GpuMemoryUsage retval;
    retval.Budget = budget.BudgetBytes;
    retval.Usage = budget.UsageBytes;

    return retval;
}


