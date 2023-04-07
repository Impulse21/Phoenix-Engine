#pragma once

// Credit to Raptor Engine: Using it for learning.



#define PhxKB(size)                 (size * 1024)
#define PhxMB(size)                 (size * 1024 * 1024)
#define PhxGB(size)                 (size * 1024 * 1024 * 1024)

#define PhxToKB(x)	((size_t) (x) >> 10)
#define PhxToMB(x)	((size_t) (x) >> 20)
#define PhxToGB(x)	((size_t) (x) >> 30)

namespace PhxEngine::Core
{
	struct MemoryStatistics 
	{
		size_t AllocatedBytes;
		size_t TotalBytes;
		uint32_t AllocationCount;

		void add(size_t a) 
		{
			if (a)
			{
				this->AllocatedBytes += a;
				++this->AllocationCount;
			}
		}
	};

	class IAllocator
	{
	public:
		virtual ~IAllocator() = default;

		virtual void* Allocate(size_t size, size_t alignment) = 0;
		virtual void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) = 0;

		virtual void Deallocate(void* pointer) = 0;
	};

	class HeapAllocator : public IAllocator
	{
	public:
		void Initialize(size_t size);
		void Finalize();

		void BuildIU();


		void* Allocate(size_t size, size_t alignment) override;
		void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) override;

		void Deallocate(void* pointer) override;

	private:
		void* m_tlsfHandle;
		void* m_memory;
		size_t m_allocatedSize = 0;
		size_t m_maxSize = 0;
	};

	class StackAllocator : public IAllocator
	{
	public:
		void Initialize(size_t size);
		void Finalize();

		void* Allocate(size_t size, size_t alignment) override;
		void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) override;

		void Deallocate(void* pointer) override;

		size_t GetMarker();
		void FreeMarker(size_t marker);

		void Clear();

	private:
		uint8_t* m_memory = nullptr;
		size_t m_totalSize = 0;
		size_t m_allocatedSize = 0;
	};

	class LinearAllocator: public IAllocator
	{
	public:
		void Initialize(size_t size);
		void Finalize();

		void* Allocate(size_t size, size_t alignment) override;
		void* Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum) override;

		void Deallocate(void* pointer) override;

		void Clear();

	private:
		uint8_t* m_memory = nullptr;
		size_t m_totalSize = 0;
		size_t m_allocatedSize = 0;
	};

	struct MemoryServiceConfiguration
	{
		size_t MaximumDynamicSize = PhxMB(32);
	};

	class MemoryService
	{
	public:
		static MemoryService& GetInstance();
		void Initialize(PhxEngine::Core::MemoryServiceConfiguration const& config);
		void Finalize();

		void BuildUI();

		LinearAllocator& GetScratchAllocator() { return this->m_scratchAllocator; }
		HeapAllocator& GetSystemAllocator() { return this->m_systemAllocator; }

	private:
		MemoryService() = default;

	private:
		LinearAllocator m_scratchAllocator;
		HeapAllocator m_systemAllocator;

	};
}

