#include "pch.h"
#include "phxD3D12UploadBuffer.h"

#include "phxRhi_D3D12.h"

using namespace phx::rhi;
using namespace phx::rhi::d3d12;

void UploadBuffer::Initialize(D3D12GfxDevice* device, size_t pageSize)
{
	this->m_gfxDevice = device;
	this->m_pageSize = pageSize;
	this->m_currentPage = nullptr;
}

struct Allocation
{
	void* CpuData;
	D3D12_GPU_VIRTUAL_ADDRESS Gpu;
	BufferHandle BufferHandle;
	size_t Offset;
};

UploadAllocation UploadBuffer::Allocate(size_t sizeInBytes, size_t alignment)
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
		page = std::make_shared<Page>(this->m_gfxDevice, this->m_pageSize);
		this->m_pagePool.push_back(page);
	}

	return page;
}
void UploadBuffer::Reset()
{
	if (this->m_currentPage)
		this->m_currentPage.reset();

	this->m_availablePages = this->m_pagePool;
	for (auto page : this->m_availablePages)
	{
		page->Reset();
	}
}


UploadBuffer::Page::Page(D3D12GfxDevice* device, size_t sizeInBytes)
	: m_gfxDevice(device)
	, m_offset(0)
	, m_pageSize(sizeInBytes)
	, m_gpuPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
	BufferDesc desc = {};
	desc.Usage = Usage::Upload;
	desc.Stride = sizeInBytes;
	desc.NumElements = 1;
	desc.InitialState = ResourceStates::CopySource | ResourceStates::GenericRead;
	desc.Binding |= BindingFlags::ShaderResource;
	// desc.IsTypeRaw = true;
	desc.DebugName = "Frame Upload Buffer";
	this->m_buffer = device->CreateBuffer(desc);

	D3D12Buffer* bufferImpl = device->GetBufferPool().Get(this->m_buffer);

	this->m_gpuPtr = bufferImpl->D3D12Resource->GetGPUVirtualAddress();
}

phx::rhi::d3d12::UploadBuffer::Page::~Page()
{
	if (this->m_buffer.IsValid())
	{
		this->m_gfxDevice->DeleteBuffer(this->m_buffer);
	}

	this->m_gpuPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
	size_t sizeInBytesAligned = MemoryAlign(sizeInBytes, alignment);
	size_t alignedOffset = MemoryAlign(this->m_offset, alignment);

	return (alignedOffset + sizeInBytesAligned) <= this->m_pageSize;
}

UploadAllocation UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (!this->HasSpace(sizeInBytes, alignment))
	{
		throw std::bad_alloc();
	}
	D3D12Buffer* bufferImpl = this->m_gfxDevice->GetBufferPool().Get(this->m_buffer);
	size_t sizeInBytesAligned = MemoryAlign(sizeInBytes, alignment);

	this->m_offset = MemoryAlign(this->m_offset, alignment);
	UploadAllocation allocation = {};
	allocation.CpuData = static_cast<uint8_t*>(bufferImpl->MappedData) + this->m_offset;
	allocation.Gpu = this->m_gpuPtr + this->m_offset;
	allocation.Offset = this->m_offset;
	allocation.BufferHandle = this->m_buffer;

	this->m_offset += sizeInBytesAligned;

	return allocation;
}

void UploadBuffer::Page::Reset()
{
	this->m_offset = 0;
}