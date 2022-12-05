#include "phxpch.h"
#include "UploadBuffer.h"
#include "GraphicsDevice.h"
using namespace PhxEngine::RHI::Dx12;

size_t AlignUp(size_t value, size_t alignment)
{
	size_t remainder = value % alignment;
	return remainder ? value + (alignment - remainder) : value;
}

UploadBuffer::UploadBuffer(GraphicsDevice& device, size_t pageSize)
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

UploadBuffer::Page::Page(GraphicsDevice& device, size_t sizeInBytes)
	: m_device(device)
	, m_offset(0)
	, m_pageSize(sizeInBytes)
	, m_gpuPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
	BufferDesc desc = {};
	desc.Usage = Usage::Upload;
	desc.SizeInBytes = sizeInBytes;
	desc.InitialState = ResourceStates::CopySource | ResourceStates::GenericRead;
	desc.Binding |= BindingFlags::ShaderResource;
	desc.MiscFlags |= BufferMiscFlags::Bindless | BufferMiscFlags::Raw;
	desc.CreateBindless = true;
	this->m_buffer = device.CreateBuffer(desc);

	Dx12Buffer* bufferImpl = device.GetBufferPool().Get(this->m_buffer);

	this->m_gpuPtr = bufferImpl->D3D12Resource->GetGPUVirtualAddress();
}

UploadBuffer::Page::~Page()
{
	if (this->m_buffer.IsValid())
	{
		this->m_device.GetBufferPool().Release(this->m_buffer);
	}

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
	Dx12Buffer* bufferImpl = this->m_device.GetBufferPool().Get(this->m_buffer);
	size_t sizeInBytesAligned = AlignUp(sizeInBytes, alignment);

	this->m_offset = AlignUp(this->m_offset, alignment);
	Allocation allocation = {};
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
