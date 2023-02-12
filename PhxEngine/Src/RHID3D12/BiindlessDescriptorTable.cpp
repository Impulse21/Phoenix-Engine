#include "phxpch.h"
#include "BiindlessDescriptorTable.h"

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

DescriptorIndex BindlessDescriptorTable::Allocate()
{
	return this->m_descriptorIndexPool.Allocate();
}

void BindlessDescriptorTable::Free(DescriptorIndex index)
{
	this->m_descriptorIndexPool.Release(index);
}
