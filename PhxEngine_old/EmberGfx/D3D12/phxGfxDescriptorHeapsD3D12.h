#pragma once

#include "pch.h"
#include <set>
#include <map>
#include <mutex>

namespace phx::gfx
{
	class DescriptorHeapAllocation;
	class DescriptorHeapAllocationPage;
	class IDescriptorAllocator
	{
	public:
		virtual ~IDescriptorAllocator() = default;

		virtual DescriptorHeapAllocation Allocate(uint32_t count) = 0;
		virtual void Free(DescriptorHeapAllocation&& allocation) = 0;
		virtual uint32_t GetDescriptorSize() const = 0;
	};

	struct D3D12TypedCpuDescriptorHandle : public D3D12_CPU_DESCRIPTOR_HANDLE
	{
		D3D12TypedCpuDescriptorHandle() = default;
		D3D12TypedCpuDescriptorHandle(const D3D12TypedCpuDescriptorHandle& other) { ptr = other.ptr; }

		D3D12TypedCpuDescriptorHandle operator =(const D3D12TypedCpuDescriptorHandle& other)
		{
			ptr = other.ptr;
			return *this;
		}

	};

	class DescriptorHeapAllocation
	{
	public:
		DescriptorHeapAllocation() noexcept
			: m_numHandles(1)
		{
			m_firstCpuHandle.ptr = 0;
			m_firstGpuHandle.ptr = 0;
		}

		DescriptorHeapAllocation(
			uint32_t pageId,
			IDescriptorAllocator* descriptorAllocator,
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

		DescriptorHeapAllocation(DescriptorHeapAllocation&& other) noexcept
			: m_firstCpuHandle(std::move(other.m_firstCpuHandle))
			, m_firstGpuHandle(std::move(other.m_firstGpuHandle))
			, m_descriptorAllocator(std::move(other.m_descriptorAllocator))
			, m_pageId(std::move(other.m_pageId))
			, m_numHandles(std::move(other.m_numHandles))
			, m_descriptorSize(std::move(other.m_descriptorSize))
		{
			other.Reset();
		}

		DescriptorHeapAllocation& operator=(DescriptorHeapAllocation&& other) noexcept
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

		DescriptorHeapAllocation(DescriptorHeapAllocation const& other) noexcept
		{
			this->m_firstCpuHandle = other.m_firstCpuHandle;
			this->m_firstGpuHandle = other.m_firstGpuHandle;
			this->m_descriptorAllocator = other.m_descriptorAllocator;
			this->m_pageId = other.m_pageId;
			this->m_numHandles = other.m_numHandles;
			this->m_descriptorSize = other.m_descriptorSize;
		}

		DescriptorHeapAllocation& operator=(DescriptorHeapAllocation const& other)
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
		IDescriptorAllocator* m_descriptorAllocator = nullptr;
		uint32_t m_numHandles = 0;

		uint32_t m_descriptorSize = 0;
	};

	class DescriptorHeapAllocationPage
	{
	public:
		DescriptorHeapAllocationPage(
			uint32_t id,
			IDescriptorAllocator* allocator,
			Microsoft::WRL::ComPtr<ID3D12Device2> d3dDevice,
			D3D12_DESCRIPTOR_HEAP_DESC const& heapDesc,
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> d3dHeap,
			uint32_t numDescriptorsInHeap,
			uint32_t initOffset);

		DescriptorHeapAllocationPage(
			uint32_t id,
			IDescriptorAllocator* allocator,
			Microsoft::WRL::ComPtr<ID3D12Device2> d3dDevice,
			D3D12_DESCRIPTOR_HEAP_DESC const& heapDesc,
			uint32_t numDescriptorsInHeap);

		DescriptorHeapAllocation Allocate(uint32_t numDescriptors);

		void Free(DescriptorHeapAllocation&& allocation);

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
		IDescriptorAllocator* m_allocator;

		FreeListByOffset m_freeListByOffset;
		FreeListBySize m_freeListBySize;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_d3d12Heap;

		CD3DX12_CPU_DESCRIPTOR_HANDLE m_baseCpuDescritpor;
		CD3DX12_GPU_DESCRIPTOR_HANDLE m_baseGpuDescritpor;

		uint32_t m_numDescriptorsInHeap;
		uint32_t m_numFreeHandles;
		uint32_t m_descritporSize;

		std::mutex m_allocationMutex;
	};

	class CpuDescriptorHeap : public IDescriptorAllocator, NonCopyable
	{
	public:
		void Initialize(
			Microsoft::WRL::ComPtr<ID3D12Device2> device,
			uint32_t numDesctiptors,
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	public:
		DescriptorHeapAllocation Allocate(uint32_t numDescriptors) override;
		void Free(DescriptorHeapAllocation&& allocation) override;
		uint32_t GetDescriptorSize() const override { return this->m_descriptorSize; }

		void FreeAllocation(DescriptorHeapAllocation&& allocation);

	private:
		std::shared_ptr<DescriptorHeapAllocationPage> CreateAllocationPage();

	private:
		uint32_t m_descriptorSize;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_device;

		D3D12_DESCRIPTOR_HEAP_DESC m_heapDesc;
		uint32_t m_numDescriptorsPerHeap;

		using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorHeapAllocationPage>>;

		std::mutex m_heapPoolMutex;
		DescriptorHeapPool m_heapPool;
		std::set<size_t> m_availableHeaps;
	};

	class GpuDescriptorHeap : public IDescriptorAllocator, NonCopyable
	{
	public:
		void Initialize(
			Microsoft::WRL::ComPtr<ID3D12Device2> device,
			uint32_t numDesctiptors,
			uint32_t numDynamicDesciprotrs,
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			D3D12_DESCRIPTOR_HEAP_FLAGS flags);

	public:
		DescriptorHeapAllocation Allocate(uint32_t numDescriptors) override;
		void Free(DescriptorHeapAllocation&& allocation) override;

		uint32_t GetDescriptorSize() const override { return this->m_descriptorSize; }
		ID3D12DescriptorHeap* GetNativeHeap() { return this->m_d3dHeap.Get(); }
		ID3D12DescriptorHeap* GetNativeHeap() const { return this->m_d3dHeap.Get(); }

		DescriptorHeapAllocation AllocateDynamic(uint32_t numDescriptors);

	public:
		void FreeAllocation(DescriptorHeapAllocation&& allocation);

	private:
		uint32_t m_descriptorSize;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_device;

		D3D12_DESCRIPTOR_HEAP_DESC m_heapDesc;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_d3dHeap;

		// Memory pages
		std::unique_ptr<DescriptorHeapAllocationPage> m_staticPage;
		std::unique_ptr<DescriptorHeapAllocationPage> m_dynamicPage;
	};

	class DynamicSuballocator final : public IDescriptorAllocator
	{
	public:
		DynamicSuballocator(
			GpuDescriptorHeap& gpuDescriptorHeap,
			uint32_t dynamicChunkSize);

		// clang-format off
		DynamicSuballocator(const DynamicSuballocator&) = delete;
		DynamicSuballocator(DynamicSuballocator&&) = delete;
		DynamicSuballocator& operator = (const DynamicSuballocator&) = delete;
		DynamicSuballocator& operator = (DynamicSuballocator&&) = delete;
		// clang-format on

		~DynamicSuballocator() = default;

		void ReleaseAllocations();
		DescriptorHeapAllocation Allocate(uint32_t numDescriptors) override;
		void Free(DescriptorHeapAllocation&& allocation) override;
		uint32_t GetDescriptorSize() const override { return this->m_parentGpuHeap.GetDescriptorSize(); }

	private:
		GpuDescriptorHeap& m_parentGpuHeap;
		std::vector<DescriptorHeapAllocation> m_subAllocations;

		uint32_t m_currentSuballocationOffset = 0;
		uint32_t m_dynamicChunkSize = 0;

		uint32_t m_currentDescriptorCount = 0;
		uint32_t m_peakDescriptorCount = 0;
		uint32_t m_currentSuballocationTotalSize = 0;
		uint32_t m_peakSuballocationTotalSize = 0;
	};
}