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

void PhxEngine::RHI::D3D12::D3D12GpuMemoryAllocator::AllocateBuffer(BufferDesc const& desc)
{
	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
	if ((desc.Binding & RHI::BindingFlags::UnorderedAccess) == RHI::BindingFlags::UnorderedAccess)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	switch (desc.Usage)
	{
	case Usage::ReadBack:
		allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;

		initialState = D3D12_RESOURCE_STATE_COPY_DEST;
		break;

	case Usage::Upload:
		allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
		break;

	case Usage::Default:
	default:
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		initialState = D3D12_RESOURCE_STATE_COMMON;
	}

	UINT64 alignedSize = 0;
	if ((desc.Binding & BindingFlags::ConstantBuffer) == BindingFlags::ConstantBuffer)
	{
		alignedSize = Core::AlignTo(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.Stride * desc.NumElements, resourceFlags, alignedSize);

	/* TODO: Add Sparse stuff
	if (resource_heap_tier >= D3D12_RESOURCE_HEAP_TIER_2)
	{
		// tile pool memory can be used for sparse buffers and textures alike (requires resource heap tier 2):
		allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
	}
	else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_BUFFER))
	{
		allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	}
	else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_NON_RT_DS))
	{
		allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
	}
	else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_RT_DS))
	{
		allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
	}
	*/

	if ((outBuffer.Desc.MiscFlags & BufferMiscFlags::IsAliasedResource) == BufferMiscFlags::IsAliasedResource)
	{
		D3D12_RESOURCE_ALLOCATION_INFO finalAllocInfo = {};
		finalAllocInfo.Alignment = 0;
		finalAllocInfo.SizeInBytes = Core::AlignTo(
			outBuffer.Desc.Stride * outBuffer.Desc.NumElements,
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * 1024);

		ThrowIfFailed(
			this->m_d3d12MemAllocator->AllocateMemory(
				&allocationDesc,
				&finalAllocInfo,
				&outBuffer.Allocation));
		return;
	}
	else if (outBuffer.Desc.AliasedBuffer.IsValid())
	{
		D3D12Buffer* aliasedBuffer = this->m_bufferPool.Get(outBuffer.Desc.AliasedBuffer);

		ThrowIfFailed(
			this->m_d3d12MemAllocator->CreateAliasingResource(
				aliasedBuffer->Allocation.Get(),
				0,
				&resourceDesc,
				initialState,
				nullptr,
				IID_PPV_ARGS(&outBuffer.D3D12Resource)));
	}
	else
	{
		ThrowIfFailed(
			this->m_d3d12MemAllocator->CreateResource(
				&allocationDesc,
				&resourceDesc,
				initialState,
				nullptr,
				&outBuffer.Allocation,
				IID_PPV_ARGS(&outBuffer.D3D12Resource)));
	}
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


