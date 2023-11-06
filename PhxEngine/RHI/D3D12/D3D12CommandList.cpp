#include "D3D12CommandList.h"

using namespace PhxEngine::RHI::D3D12;
void D3D12CommandList::Reset(ID3D12CommandAllocator* allocator)
{
	this->NativeCmdList->Reset(allocator, nullptr);
	this->Allocator = allocator;

}
