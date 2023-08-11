
#include "D3D12BiindlessDescriptorTable.h"

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

void PhxEngine::RHI::D3D12::D3D12BindlessDescriptorTable::Initialize(DescriptorHeapAllocation&& allocation)
{
	this->m_allocation = std::move(allocation);
}

DescriptorIndex D3D12BindlessDescriptorTable::Allocate()
{
	return this->m_descriptorIndexPool.Allocate();
}

void D3D12BindlessDescriptorTable::Free(DescriptorIndex index)
{
	this->m_descriptorIndexPool.Release(index);
}
