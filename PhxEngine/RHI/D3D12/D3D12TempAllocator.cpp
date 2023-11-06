#include "D3D12TempAllocator.h"

#include "D3D12ResourceManager.h"

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;



void D3D12TempAllocator::Initialize(size_t pageSize)
{
	this->m_pageSize = pageSize;
}

void PhxEngine::RHI::D3D12::D3D12TempAllocator::Finalize()
{
	this->m_currentPage.reset();
	this->m_availablePages.clear();
	this->m_pagePool.clear();
	this->m_pageSize = 0;
}

struct Allocation
{
	void* CpuData;
	D3D12_GPU_VIRTUAL_ADDRESS Gpu;
	BufferHandle BufferHandle;
	size_t Offset;
};

TempAllocation D3D12TempAllocator::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (sizeInBytes > this->m_pageSize)
	{
		throw std::bad_alloc();
	}

	if (!this->m_currentPage || !this->m_currentPage->HasSpace(sizeInBytes, alignment))
	{
		this->m_currentPage = this->RequestPage();
	}

	return this->m_currentPage->Allocate(sizeInBytes, alignment);
}

std::shared_ptr<D3D12TempAllocator::Page> D3D12TempAllocator::RequestPage()
{
	std::shared_ptr<Page> page;
	if (!this->m_availablePages.empty())
	{
		page = this->m_availablePages.front();
		this->m_availablePages.pop_front();
	}
	else
	{
		page = std::make_shared<Page>(this->m_pageSize);
		this->m_pagePool.push_back(page);
	}

	return page;
}

void D3D12TempAllocator::Reset()
{
	this->m_currentPage.reset();

	this->m_availablePages = this->m_pagePool;
	for (auto page : this->m_availablePages)
	{
		page->Reset();
	}
}


D3D12TempAllocator::Page::Page(size_t sizeInBytes)
	: m_offset(0)
	, m_pageSize(sizeInBytes)
	, m_gpuPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
	BufferDesc desc = {};
	desc.Usage = Usage::Upload;
	desc.Stride = sizeInBytes;
	desc.NumElements = 1;
	desc.InitialState = ResourceStates::CopySource | ResourceStates::GenericRead;
	desc.Binding |= BindingFlags::ShaderResource;
	desc.MiscFlags |= BufferMiscFlags::Bindless | BufferMiscFlags::Raw;
	desc.DebugName = "Frame Upload Buffer";
	this->m_buffer = D3D12ResourceManager::GPtr->CreateGpuBuffer(desc);

	D3D12Buffer* bufferImpl = D3D12ResourceManager::GPtr->GetBufferPool().Get(this->m_buffer);

	this->m_gpuPtr = bufferImpl->D3D12Resource->GetGPUVirtualAddress();
}

PhxEngine::RHI::D3D12::D3D12TempAllocator::Page::~Page()
{
	if (this->m_buffer.IsValid() && D3D12ResourceManager::GPtr)
	{
		D3D12ResourceManager::GPtr->DeleteGpuBuffer(this->m_buffer);
	}

	this->m_gpuPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool D3D12TempAllocator::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
	size_t sizeInBytesAligned = Core::AlignUp(sizeInBytes, alignment);
	size_t alignedOffset = Core::AlignUp(this->m_offset, alignment);

	return (alignedOffset + sizeInBytesAligned) <= this->m_pageSize;
}

TempAllocation D3D12TempAllocator::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (!this->HasSpace(sizeInBytes, alignment))
	{
		throw std::bad_alloc();
	}
	D3D12Buffer* bufferImpl = D3D12ResourceManager::GPtr->GetBufferPool().Get(this->m_buffer);
	size_t sizeInBytesAligned = Core::AlignUp(sizeInBytes, alignment);

	this->m_offset = Core::AlignUp(this->m_offset, alignment);
	TempAllocation allocation = {};
	allocation.Data = static_cast<uint8_t*>(bufferImpl->MappedData) + this->m_offset;
	allocation.Gpu = this->m_gpuPtr + this->m_offset;
	allocation.ByteOffset = this->m_offset;
	allocation.BufferHandle = this->m_buffer;

	this->m_offset += sizeInBytesAligned;

	return allocation;
}

void D3D12TempAllocator::Page::Reset()
{
	this->m_offset = 0;
}

