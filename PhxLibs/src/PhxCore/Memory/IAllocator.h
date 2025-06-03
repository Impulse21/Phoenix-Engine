#pragma once
#include <cstdint>

namespace phx
{

	struct NonCopyable
	{
		NonCopyable& operator=(const NonCopyable&) = delete;
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable() = default;
	};

	class IAllocator : NonCopyable
	{
	public:
		virtual ~IAllocator() = default;

		[[nodiscard]] virtual void* Allocate(size_t size, size_t alignment) = 0;
		[[nodiscard]] virtual void* Allocate(size_t size, size_t alignment, const char* file, int32_t line) = 0;

		virtual void Deallocate(void* pointer) = 0;
	};
}