#include "pch.h"
#include "phxGfxD3D12DescriptorHeaps.h"

#include <memory>

using namespace phx::gfx;

void phx::gfx::CpuDescriptorHeap::Initialize(
	Microsoft::WRL::ComPtr<ID3D12Device2> device,
	uint32_t numDesctiptors,
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	this->m_heapDesc =
	{
		type,
		numDesctiptors,
		flags,
		1 // node mask
	};

	this->m_descriptorSize = device->GetDescriptorHandleIncrementSize(type);
	this->m_numDescriptorsPerHeap = numDesctiptors;
	this->m_device = device;
}

DescriptorHeapAllocation CpuDescriptorHeap::Allocate(uint32_t numDescriptors)
{
	std::scoped_lock _(this->m_heapPoolMutex);

	DescriptorHeapAllocation allocation;
	for (auto iter = this->m_availableHeaps.begin(); iter != this->m_availableHeaps.end(); iter++)
	{
		auto allocationPage = this->m_heapPool[*iter];

		allocation = allocationPage->Allocate(numDescriptors);

		if (allocationPage->GetNumFreeHandles() == 0)
		{
			iter = this->m_availableHeaps.erase(iter);
		}

		if (!allocation.IsNull())
		{
			// We found an allocation \0/
			break;
		}
	}

	// We never found anything so lets go a head and create a new page
	if (allocation.IsNull())
	{
		// TODO: maybe log here that we are increasing the size.
		this->m_numDescriptorsPerHeap = std::max(this->m_numDescriptorsPerHeap, numDescriptors);
		auto newPage = this->CreateAllocationPage();

		allocation = newPage->Allocate(numDescriptors);

	}

	return allocation;
}

void CpuDescriptorHeap::Free(DescriptorHeapAllocation&& allocation)
{
	this->FreeAllocation(std::move(allocation));
}

std::shared_ptr<DescriptorHeapAllocationPage> CpuDescriptorHeap::CreateAllocationPage()
{
#pragma warning( push )
#pragma warning( disable : 4267)
	auto newPage = std::make_shared<DescriptorHeapAllocationPage>(
		this->m_heapPool.size(),
		this,
		this->m_device,
		this->m_heapDesc,
		this->m_numDescriptorsPerHeap);

#pragma warning( pop )
	this->m_heapPool.push_back(newPage);
	this->m_availableHeaps.insert(this->m_heapPool.size() - 1);

	return newPage;
}

void CpuDescriptorHeap::FreeAllocation(DescriptorHeapAllocation&& allocation)
{
	uint32_t pageId = allocation.GetPageId();
	if (pageId >= this->m_heapPool.size())
	{
		return;
	}

	this->m_heapPool[pageId]->Free(std::move(allocation));
}

DescriptorHeapAllocationPage::DescriptorHeapAllocationPage(
	uint32_t id,
	IDescriptorAllocator* allocator,
	Microsoft::WRL::ComPtr<ID3D12Device2> d3dDevice,
	D3D12_DESCRIPTOR_HEAP_DESC const& heapDesc,
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> d3dHeap,
	uint32_t numDescriptorsInHeap,
	uint32_t initOffset)
	: m_id(id)
	, m_allocator(allocator)
	, m_heapDesc(heapDesc)
	, m_numDescriptorsInHeap(numDescriptorsInHeap)
	, m_numFreeHandles(numDescriptorsInHeap)
	, m_d3d12Heap(d3dHeap)
{
	this->m_descritporSize = d3dDevice->GetDescriptorHandleIncrementSize(this->m_heapDesc.Type);

	this->m_baseCpuDescritpor = this->m_d3d12Heap->GetCPUDescriptorHandleForHeapStart();
	this->m_baseCpuDescritpor.ptr += this->GetDescriptorSize() * initOffset;

	this->m_baseGpuDescritpor = this->m_d3d12Heap->GetGPUDescriptorHandleForHeapStart();
	this->m_baseGpuDescritpor.ptr += this->GetDescriptorSize() * initOffset;

	// Initialize the free lists
	this->AddNewBlock(0, this->m_numFreeHandles);
}

DescriptorHeapAllocationPage::DescriptorHeapAllocationPage(
	uint32_t id,
	IDescriptorAllocator* allocator,
	Microsoft::WRL::ComPtr<ID3D12Device2> d3dDevice,
	D3D12_DESCRIPTOR_HEAP_DESC const& heapDesc,
	uint32_t numDescriptorsInHeap)
	: m_id(id)
	, m_allocator(allocator)
	, m_heapDesc(heapDesc)
	, m_numDescriptorsInHeap(numDescriptorsInHeap)
	, m_numFreeHandles(numDescriptorsInHeap)
{
	dx::ThrowIfFailed(
		d3dDevice->CreateDescriptorHeap(
			&this->m_heapDesc,
			IID_PPV_ARGS(&this->m_d3d12Heap)));

	this->m_descritporSize = d3dDevice->GetDescriptorHandleIncrementSize(this->m_heapDesc.Type);

	this->m_baseCpuDescritpor = this->m_d3d12Heap->GetCPUDescriptorHandleForHeapStart();

	if (this->IsShaderVisibile())
	{
		this->m_baseGpuDescritpor = this->m_d3d12Heap->GetGPUDescriptorHandleForHeapStart();
	}

	// Initialize the free lists
	this->AddNewBlock(0, this->m_numFreeHandles);
}

DescriptorHeapAllocation DescriptorHeapAllocationPage::Allocate(uint32_t numDescriptors)
{
	std::scoped_lock _(this->m_allocationMutex);
	if (numDescriptors > this->m_numFreeHandles)
	{
		return DescriptorHeapAllocation();
	}

	// get the first block that is larget enough to statisfy
	auto smallestBlockIter = this->m_freeListBySize.lower_bound(numDescriptors);
	if (smallestBlockIter == this->m_freeListBySize.end())
	{
		return DescriptorHeapAllocation();
	}

	auto blockSize = smallestBlockIter->first;

	auto offsetIter = smallestBlockIter->second;

	auto offset = offsetIter->first;

	this->m_freeListBySize.erase(smallestBlockIter);
	this->m_freeListByOffset.erase(offsetIter);

	auto newOffset = offset + numDescriptors;
	auto newSize = blockSize - numDescriptors;

	if (newSize > 0)
	{
		this->AddNewBlock(newOffset, newSize);
	}

	this->m_numFreeHandles -= numDescriptors;
	D3D12_CPU_DESCRIPTOR_HANDLE  cpuHandle = this->m_baseCpuDescritpor;
	cpuHandle.ptr += offset * this->m_descritporSize;

	D3D12_GPU_DESCRIPTOR_HANDLE  gpuHandle = this->m_baseGpuDescritpor;
	if (this->m_heapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		gpuHandle.ptr += offset * this->m_descritporSize;
	}

	return DescriptorHeapAllocation(
		this->m_id,
		this->m_allocator,
		cpuHandle,
		gpuHandle,
		numDescriptors);
}

void DescriptorHeapAllocationPage::Free(DescriptorHeapAllocation&& allocation)
{
	std::scoped_lock _(this->m_allocationMutex);

	auto offset = this->ComputeCpuOffset(allocation.GetCpuHandle());
	this->FreeBlock(offset, allocation.GetNumHandles());

	// Reset so we don't repeat the process of safe releasing the allocation.
	// this is the finaly breathe of the allocation.
	allocation.Reset();
}

uint32_t DescriptorHeapAllocationPage::ComputeCpuOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	return static_cast<uint32_t>(handle.ptr - this->m_baseCpuDescritpor.ptr) / this->m_descritporSize;
}

void DescriptorHeapAllocationPage::AddNewBlock(uint32_t offset, uint32_t numDescriptors)
{
	auto offsetIt = this->m_freeListByOffset.emplace(offset, numDescriptors);
	auto sizeIt = this->m_freeListBySize.emplace(numDescriptors, offsetIt.first);
	offsetIt.first->second.FreeListBySizeIt = sizeIt;
}

void DescriptorHeapAllocationPage::FreeBlock(uint32_t offset, uint32_t numDescriptors)
{
	// Find the first element whose offset is greater than the specified offset.
	// This is the block that should appear after the block that is being freed.
	auto nextBlockIt = this->m_freeListByOffset.upper_bound(offset);

	// Find the block that appears before the block being freed.
	auto prevBlockIt = nextBlockIt;

	// If it's not the first block in the list.
	if (prevBlockIt != this->m_freeListByOffset.begin())
	{
		// Go to the previous block in the list.
		--prevBlockIt;
	}
	else
	{
		// Otherwise, just set it to the end of the list to indicate that no
		// block comes before the one being freed.
		prevBlockIt = this->m_freeListByOffset.end();
	}

	// Add the number of free handles back to the heap.
	// This needs to be done before merging any blocks since merging
	// blocks modifies the numDescriptors variable.
	this->m_numFreeHandles += numDescriptors;

	if (prevBlockIt != this->m_freeListByOffset.end() &&
		offset == prevBlockIt->first + prevBlockIt->second.Size)
	{
		// The previous block is exactly behind the block that is to be freed.
		//
		// PrevBlock.Offset           Offset
		// |                          |
		// |<-----PrevBlock.Size----->|<------Size-------->|
		//

		// Increase the block size by the size of merging with the previous block.
		offset = prevBlockIt->first;
		numDescriptors += prevBlockIt->second.Size;

		// Remove the previous block from the free list.
		this->m_freeListBySize.erase(prevBlockIt->second.FreeListBySizeIt);
		this->m_freeListByOffset.erase(prevBlockIt);
	}

	if (nextBlockIt != this->m_freeListByOffset.end() &&
		offset + numDescriptors == nextBlockIt->first)
	{
		// The next block is exactly in front of the block that is to be freed.
		//
		// Offset               NextBlock.Offset 
		// |                    |
		// |<------Size-------->|<-----NextBlock.Size----->|

		// Increase the block size by the size of merging with the next block.
		numDescriptors += nextBlockIt->second.Size;

		// Remove the next block from the free list.
		this->m_freeListBySize.erase(nextBlockIt->second.FreeListBySizeIt);
		this->m_freeListByOffset.erase(nextBlockIt);
	}

	this->AddNewBlock(offset, numDescriptors);
}

void phx::gfx::GpuDescriptorHeap::Initialize(
	Microsoft::WRL::ComPtr<ID3D12Device2> device,
	uint32_t numDesctiptors,
	uint32_t numDynamicDesciprotrs,
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	this->m_device = device;
	this->m_descriptorSize = this->m_device->GetDescriptorHandleIncrementSize(type);

	this->m_heapDesc =
	{
		type,
		numDesctiptors + numDynamicDesciprotrs,
		flags,
		1 // node mask
	};

	dx::ThrowIfFailed(
		this->m_device->CreateDescriptorHeap(
			&this->m_heapDesc,
			IID_PPV_ARGS(&this->m_d3dHeap)));


	this->m_staticPage = std::make_unique<DescriptorHeapAllocationPage>(
		0,
		this,
		this->m_device,
		this->m_heapDesc,
		this->m_d3dHeap,
		numDesctiptors,
		0);

	this->m_dynamicPage = std::make_unique<DescriptorHeapAllocationPage>(
		1,
		this,
		this->m_device,
		this->m_heapDesc,
		this->m_d3dHeap,
		numDynamicDesciprotrs,
		numDesctiptors + 1);
}

DescriptorHeapAllocation GpuDescriptorHeap::Allocate(uint32_t numDescriptors)
{
	return this->m_staticPage->Allocate(numDescriptors);
}

void GpuDescriptorHeap::Free(DescriptorHeapAllocation&& allocation)
{
	this->FreeAllocation(std::move(allocation));
}

DescriptorHeapAllocation GpuDescriptorHeap::AllocateDynamic(uint32_t numDescriptors)
{
	return this->m_dynamicPage->Allocate(numDescriptors);
}

void GpuDescriptorHeap::FreeAllocation(DescriptorHeapAllocation&& allocation)
{
	auto pageId = allocation.GetPageId();

	assert(pageId == 0 || pageId == 1);

	if (pageId == 0)
	{
		this->m_staticPage->Free(std::move(allocation));
	}
	else
	{
		this->m_dynamicPage->Free(std::move(allocation));
	}
}

void DynamicSuballocator::ReleaseAllocations()
{
	for (auto& allocation : this->m_subAllocations)
	{
		this->m_parentGpuHeap.Free(std::move(allocation));
	}

	this->m_subAllocations.clear();
	this->m_currentDescriptorCount = 0;
	this->m_currentSuballocationTotalSize = 0;
}

DescriptorHeapAllocation DynamicSuballocator::Allocate(uint32_t numDescriptors)
{

	if (this->m_subAllocations.empty() ||
		this->m_currentSuballocationOffset + numDescriptors > this->m_subAllocations.back().GetNumHandles())
	{
		auto subAllocationSize = std::max(numDescriptors, this->m_dynamicChunkSize);
		auto newAllocation = this->m_parentGpuHeap.AllocateDynamic(subAllocationSize);

		if (newAllocation.IsNull())
		{
			// LOG_CORE_ERROR("Dynamic space is out of GPU Descriptors");
			return newAllocation;
		}

		this->m_subAllocations.emplace_back(std::move(newAllocation));
		this->m_currentSuballocationOffset = 0;
		this->m_currentSuballocationTotalSize += subAllocationSize;
		this->m_peakSuballocationTotalSize = std::max(this->m_peakSuballocationTotalSize, this->m_currentSuballocationTotalSize);
	}

	auto& currentAllocation = this->m_subAllocations.back();

	uint32_t pageId = currentAllocation.GetPageId();

	DescriptorHeapAllocation allocation(
		pageId,
		this,
		currentAllocation.GetCpuHandle(this->m_currentSuballocationOffset),
		currentAllocation.GetGpuHandle(this->m_currentSuballocationOffset),
		numDescriptors);

	this->m_currentSuballocationOffset += numDescriptors;
	this->m_currentDescriptorCount += numDescriptors;
	this->m_peakDescriptorCount = std::max(this->m_peakDescriptorCount, numDescriptors);

	return std::move(allocation);
}

void DynamicSuballocator::Free(DescriptorHeapAllocation&& allocation)
{
	allocation.Reset();
}

DynamicSuballocator::DynamicSuballocator(
	GpuDescriptorHeap& gpuDescriptorHeap,
	uint32_t dynamicChunkSize)
	: m_parentGpuHeap(gpuDescriptorHeap)
	, m_dynamicChunkSize(dynamicChunkSize)
{

}