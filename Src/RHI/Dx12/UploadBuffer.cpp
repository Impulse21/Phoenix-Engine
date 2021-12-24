#include "UploadBuffer.h"

using namespace PhxEngine::RHI::Dx12;

size_t AlignUp(size_t value, size_t alignment)
{
	size_t remainder = value % alignment;
	return remainder ? value + (alignment - remainder) : value;
}

UploadBuffer::UploadBuffer(RefCountPtr<ID3D12Device2> device, size_t pageSize)
	: m_device(device)
	, m_pageSize(pageSize)
{
}

UploadBuffer::Allocation UploadBuffer::Allocate(size_t sizeInBytes, size_t alignment)
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

void UploadBuffer::Reset()
{
	this->m_currentPage.reset();

	this->m_availablePages = this->m_pagePool;
	for (auto page : this->m_availablePages)
	{
		page->Reset();
	}
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
	std::shared_ptr<Page> page;
	if (!this->m_availablePages.empty())
	{
		page = this->m_availablePages.front();
		this->m_availablePages.pop_front();
	}
	else
	{
		page = std::make_shared<Page>(this->m_device, this->m_pageSize);
		this->m_pagePool.push_back(page);
	}

	return page;
}

UploadBuffer::Page::Page(RefCountPtr<ID3D12Device2> device, size_t sizeInBytes)
	: m_cpuPtr(nullptr)
	, m_offset(0)
	, m_pageSize(sizeInBytes)
	, m_gpuPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
	ThrowIfFailed(
		device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&this->m_d3dResource)));

	this->m_gpuPtr = this->m_d3dResource->GetGPUVirtualAddress();
	this->m_d3dResource->Map(0, nullptr, &this->m_cpuPtr);
}

UploadBuffer::Page::~Page()
{
	this->m_d3dResource->Unmap(0, nullptr);
	this->m_cpuPtr = nullptr;
	this->m_gpuPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
	size_t sizeInBytesAligned = AlignUp(sizeInBytes, alignment);
	size_t alignedOffset = AlignUp(this->m_offset, alignment);

	return (alignedOffset + sizeInBytesAligned) <= this->m_pageSize;
}

UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (!this->HasSpace(sizeInBytes, alignment))
	{
		throw std::bad_alloc();
	}

	size_t sizeInBytesAligned = AlignUp(sizeInBytes, alignment);

	this->m_offset = AlignUp(this->m_offset, alignment);
	Allocation allocation = {};
	allocation.Cpu = static_cast<uint8_t*>(this->m_cpuPtr) + this->m_offset;
	allocation.Gpu = this->m_gpuPtr + this->m_offset;

	this->m_offset += sizeInBytesAligned;

	return allocation;
}

void UploadBuffer::Page::Reset()
{
	this->m_offset = 0;
}
