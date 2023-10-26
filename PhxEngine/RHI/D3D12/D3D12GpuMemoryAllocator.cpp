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

RHI::GpuMemoryUsage PhxEngine::RHI::D3D12::D3D12GpuMemoryAllocator::GetMemoryUsage() const
{
    D3D12MA::Budget budget;
    this->m_d3d12MemAllocator->GetBudget(&budget, nullptr);

    GpuMemoryUsage retval;
    retval.Budget = budget.BudgetBytes;
    retval.Usage = budget.UsageBytes;

    return retval;
}


