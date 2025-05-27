#pragma once

namespace phx
{

	template<typename BackingAllocator>
	struct EastlAllocator
	{
		using this_type = EastlAllocator<BackingAllocator>;

		const char* Name = "phx::EastlAllocator";
		BackingAllocator* Allocator = nullptr;

		EastlAllocator() = default;
		EastlAllocator(BackingAllocator& alloc) : Allocator(&alloc) {}

		void* allocate(size_t n, int flags = 0)
		{
			return Allocator->allocate(n, 8);
		}

		void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0)
		{
			return Allocator->allocate(n, alignment);
		}

		void deallocate(void* p, size_t = 0)
		{
			Allocator->deallocate(p);
		}

		const char* get_name() const { return Name; }
		void set_name(const char* n) { Name = n; }

		bool operator==(const this_type& rhs) const { return Allocator == rhs.allocator; }
		bool operator!=(const this_type& rhs) const { return !(*this == rhs); }
	};

}