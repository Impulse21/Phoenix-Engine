#pragma once

#include "D3D12Common.h"
#include <set>
#include <map>
#include <mutex>

namespace PhxEngine::RHI::D3D12
{
	class D3D12DescriptorHeapAllocation;
	class D3D12DescriptorHeapAllocationPage;
	class D3D12Device;

	class ID3D12DescriptorAllocator
	{
	public:
		virtual ~ID3D12DescriptorAllocator() = default;

		virtual D3D12DescriptorHeapAllocation Allocate(uint32_t count) = 0;
		virtual void Free(D3D12DescriptorHeapAllocation&& allocation) = 0;
		virtual uint32_t GetDescriptorSize() const = 0;
	};

	class D3D12DescriptorHeapAllocation
	{
	public:
		D3D12DescriptorHeapAllocation() noexcept
			: m_numHandles(1)
		{
			m_firstCpuHandle.ptr = 0;
			m_firstGpuHandle.ptr = 0;
		}

		D3D12DescriptorHeapAllocation(
			uint32_t pageId,
			ID3D12DescriptorAllocator* descriptorAllocator,
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle,
			uint32_t numHandles) noexcept
			: m_pageId(pageId)
			, m_descriptorAllocator(descriptorAllocator)
			, m_firstCpuHandle(cpuHandle)
			, m_firstGpuHandle(gpuHandle)
			, m_numHandles(numHandles)
		{
			this->m_descriptorSize = m_descriptorAllocator->GetDescriptorSize();
		}

		D3D12DescriptorHeapAllocation(D3D12DescriptorHeapAllocation&& other) noexcept
			: m_firstCpuHandle(std::move(other.m_firstCpuHandle))
			, m_firstGpuHandle(std::move(other.m_firstGpuHandle))
			, m_descriptorAllocator(std::move(other.m_descriptorAllocator))
			, m_pageId(std::move(other.m_pageId))
			, m_numHandles(std::move(other.m_numHandles))
			, m_descriptorSize(std::move(other.m_descriptorSize))
		{
			other.Reset();
		}

		D3D12DescriptorHeapAllocation& operator=(D3D12DescriptorHeapAllocation&& other) noexcept
		{
			this->m_firstCpuHandle = std::move(other.m_firstCpuHandle);
			this->m_firstGpuHandle = std::move(other.m_firstGpuHandle);
			this->m_descriptorAllocator = std::move(other.m_descriptorAllocator);
			this->m_pageId = std::move(other.m_pageId);
			this->m_numHandles = std::move(other.m_numHandles);
			this->m_descriptorSize = other.m_descriptorSize;

			other.Reset();
			return *this;
		}

		D3D12DescriptorHeapAllocation(D3D12DescriptorHeapAllocation const& other) noexcept
		{
			this->m_firstCpuHandle = other.m_firstCpuHandle;
			this->m_firstGpuHandle = other.m_firstGpuHandle;
			this->m_descriptorAllocator = other.m_descriptorAllocator;
			this->m_pageId = other.m_pageId;
			this->m_numHandles = other.m_numHandles;
			this->m_descriptorSize = other.m_descriptorSize;
		}

		D3D12DescriptorHeapAllocation& operator=(D3D12DescriptorHeapAllocation const& other)
		{
			this->m_firstCpuHandle = other.m_firstCpuHandle;
			this->m_firstGpuHandle = other.m_firstGpuHandle;
			this->m_descriptorAllocator = other.m_descriptorAllocator;
			this->m_pageId = other.m_pageId;
			this->m_numHandles = other.m_numHandles;
			this->m_descriptorSize = other.m_descriptorSize;

			return *this;
		}

		void Free()
		{
			if (!this->IsNull() && this->m_descriptorAllocator)
			{
				this->m_descriptorAllocator->Free(std::move(*this));
			}
		}

	public:
		void Reset()
		{
			this->m_firstCpuHandle.ptr = 0;
			this->m_firstGpuHandle.ptr = 0;
			this->m_descriptorAllocator = nullptr;
			this->m_pageId = ~0u;
			this->m_numHandles = 0;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t offset = 0) const
		{
			assert(offset < this->m_numHandles);
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(this->m_firstCpuHandle, offset * this->m_descriptorSize);
			return handle;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t offset = 0) const
		{
			assert(offset < this->m_numHandles);
			CD3DX12_GPU_DESCRIPTOR_HANDLE handle(this->m_firstGpuHandle, offset * this->m_descriptorSize);
			return handle;
		}

		template<typename HandleType>
		HandleType GetHandle(uint32_t offset = 0) const;

		template<>
		D3D12_CPU_DESCRIPTOR_HANDLE GetHandle(uint32_t offset) const
		{
			return this->GetCpuHandle(offset);
		}

		template<>
		D3D12_GPU_DESCRIPTOR_HANDLE GetHandle(uint32_t offset) const
		{
			return this->GetGpuHandle(offset);
		}

		uint32_t GetNumHandles() const { return this->m_numHandles; }
		bool IsNull() const { return this->m_firstCpuHandle.ptr == 0; }
		bool IsShaderVisible() const { return this->m_firstGpuHandle.ptr != 0; }
		uint32_t GetDescriptorSize() const { return this->m_descriptorSize; }
		uint32_t GetPageId() const { return this->m_pageId; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_firstCpuHandle = { 0 };
		D3D12_GPU_DESCRIPTOR_HANDLE m_firstGpuHandle = { 0 };

		uint32_t m_pageId;
		ID3D12DescriptorAllocator* m_descriptorAllocator = nullptr;
		uint32_t m_numHandles = 0;

		uint32_t m_descriptorSize = 0;
	};

	class D3D12DescriptorHeapAllocationPage
	{
	public:
		D3D12DescriptorHeapAllocationPage(
			uint32_t id,
			ID3D12DescriptorAllocator* allocator,
			ID3D12Device2* d3dDevice,
			D3D12_DESCRIPTOR_HEAP_DESC const& heapDesc,
			RefCountPtr<ID3D12DescriptorHeap> d3dHeap,
			uint32_t numDescriptorsInHeap,
			uint32_t initOffset);

		D3D12DescriptorHeapAllocationPage(
			uint32_t id,
			ID3D12DescriptorAllocator* allocator,
			ID3D12Device2* d3dDevice,
			D3D12_DESCRIPTOR_HEAP_DESC const& heapDesc,
			uint32_t numDescriptorsInHeap);

		D3D12DescriptorHeapAllocation Allocate(uint32_t numDescriptors);

		void Free(D3D12DescriptorHeapAllocation&& allocation);

		D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return this->m_heapDesc.Type; }
		bool HasSpace(uint32_t numDescriptors) const { return this->m_freeListBySize.lower_bound(numDescriptors) != this->m_freeListBySize.end(); }

		uint32_t GetNumFreeHandles() const { return this->m_numFreeHandles; }
		uint32_t GetDescriptorSize() const { return this->m_descritporSize; }
		uint32_t GetId() const { return this->m_id; }
		bool IsShaderVisibile() const { return (m_heapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) != 0; }

	protected:
		uint32_t ComputeCpuOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle);

		// Adds a new block to the free list
		void AddNewBlock(uint32_t offset, uint32_t numDescriptors);

		void FreeBlock(uint32_t offset, uint32_t numDescriptors);

	private:
		using Offset = uint32_t;
		using SizeType = uint32_t;

		struct FreeBlockInfo;
		using FreeListByOffset = std::map<Offset, FreeBlockInfo>;
		using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

		struct FreeBlockInfo
		{
			SizeType Size;
			FreeListBySize::iterator FreeListBySizeIt;

			FreeBlockInfo(SizeType size)
				: Size(size)
			{}
		};

		D3D12_DESCRIPTOR_HEAP_DESC m_heapDesc;
		const uint32_t m_id;
		ID3D12DescriptorAllocator* m_allocator;

		FreeListByOffset m_freeListByOffset;
		FreeListBySize m_freeListBySize;

		RefCountPtr<ID3D12DescriptorHeap> m_d3d12Heap;

		CD3DX12_CPU_DESCRIPTOR_HANDLE m_baseCpuDescritpor;
		CD3DX12_GPU_DESCRIPTOR_HANDLE m_baseGpuDescritpor;

		uint32_t m_numDescriptorsInHeap;
		uint32_t m_numFreeHandles;
		uint32_t m_descritporSize;

		std::mutex m_allocationMutex;
	};

	class D3D12CpuDescriptorHeap : public ID3D12DescriptorAllocator
	{
	public:
		D3D12CpuDescriptorHeap(
			D3D12Device* d3dDevice,
			uint32_t numDesctiptors,
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

		D3D12CpuDescriptorHeap(const D3D12CpuDescriptorHeap&) = delete;
		D3D12CpuDescriptorHeap(D3D12CpuDescriptorHeap&&) = delete;
		D3D12CpuDescriptorHeap& operator = (const D3D12CpuDescriptorHeap&) = delete;
		D3D12CpuDescriptorHeap& operator = (D3D12CpuDescriptorHeap&&) = delete;

	public:
		D3D12DescriptorHeapAllocation Allocate(uint32_t numDescriptors) override;
		void Free(D3D12DescriptorHeapAllocation&& allocation) override;
		uint32_t GetDescriptorSize() const override { return this->m_descriptorSize; }

		void FreeAllocation(D3D12DescriptorHeapAllocation&& allocation);

	private:
		std::shared_ptr<D3D12DescriptorHeapAllocationPage> CreateAllocationPage();

	private:
		const uint32_t m_descriptorSize;
		D3D12Device* m_parentDevice;

		D3D12_DESCRIPTOR_HEAP_DESC m_heapDesc;
		uint32_t m_numDescriptorsPerHeap;

		using DescriptorHeapPool = std::vector<std::shared_ptr<D3D12DescriptorHeapAllocationPage>>;

		std::mutex m_heapPoolMutex;
		DescriptorHeapPool m_heapPool;
		std::set<size_t> m_availableHeaps;
	};

	class D3D12GpuDescriptorHeap : public ID3D12DescriptorAllocator
	{
	public:
		D3D12GpuDescriptorHeap(
			D3D12Device* parentDevice,
			uint32_t numDesctiptors,
			uint32_t numDynamicDesciprotrs,
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			D3D12_DESCRIPTOR_HEAP_FLAGS flags);

		D3D12GpuDescriptorHeap(const D3D12GpuDescriptorHeap&) = delete;
		D3D12GpuDescriptorHeap(D3D12GpuDescriptorHeap&&) = delete;
		D3D12GpuDescriptorHeap& operator = (const D3D12GpuDescriptorHeap&) = delete;
		D3D12GpuDescriptorHeap& operator = (D3D12GpuDescriptorHeap&&) = delete;

		~D3D12GpuDescriptorHeap();

	public:
		D3D12DescriptorHeapAllocation Allocate(uint32_t numDescriptors) override;
		void Free(D3D12DescriptorHeapAllocation&& allocation) override;

		uint32_t GetDescriptorSize() const override { return this->m_descriptorSize; }
		ID3D12DescriptorHeap* GetNativeHeap() { return this->m_d3dHeap.Get(); }

		D3D12DescriptorHeapAllocation AllocateDynamic(uint32_t numDescriptors);

	public:
		void FreeAllocation(D3D12DescriptorHeapAllocation&& allocation);

	private:
		const uint32_t m_descriptorSize;
		D3D12Device* m_parentDevice;

		D3D12_DESCRIPTOR_HEAP_DESC m_heapDesc;
		RefCountPtr<ID3D12DescriptorHeap> m_d3dHeap;

		// Memory pages
		std::unique_ptr<D3D12DescriptorHeapAllocationPage> m_staticPage;
		std::unique_ptr<D3D12DescriptorHeapAllocationPage> m_dynamicPage;
	};

	class D3D12DynamicSuballocator final : public ID3D12DescriptorAllocator
	{
	public:
		D3D12DynamicSuballocator(
			D3D12GpuDescriptorHeap& D3D12GpuDescriptorHeap,
			uint32_t dynamicChunkSize);

		// clang-format off
		D3D12DynamicSuballocator(const D3D12DynamicSuballocator&) = delete;
		D3D12DynamicSuballocator(D3D12DynamicSuballocator&&) = delete;
		D3D12DynamicSuballocator& operator = (const D3D12DynamicSuballocator&) = delete;
		D3D12DynamicSuballocator& operator = (D3D12DynamicSuballocator&&) = delete;
		// clang-format on

		~D3D12DynamicSuballocator() = default;

		void ReleaseAllocations();
		D3D12DescriptorHeapAllocation Allocate(uint32_t numDescriptors) override;
		void Free(D3D12DescriptorHeapAllocation&& allocation) override;
		uint32_t GetDescriptorSize() const override { return this->m_parentGpuHeap.GetDescriptorSize(); }

	private:
		D3D12GpuDescriptorHeap& m_parentGpuHeap;
		std::vector<D3D12DescriptorHeapAllocation> m_subAllocations;

		uint32_t m_currentSuballocationOffset = 0;
		uint32_t m_dynamicChunkSize = 0;

		uint32_t m_currentDescriptorCount = 0;
		uint32_t m_peakDescriptorCount = 0;
		uint32_t m_currentSuballocationTotalSize = 0;
		uint32_t m_peakSuballocationTotalSize = 0;
	};
}