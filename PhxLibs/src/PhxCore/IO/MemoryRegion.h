#pragma once

#include <memory>

namespace phx
{
	using byte = std::byte;

	template<typename T>
	class MemoryRegionView
	{
	public:
		MemoryRegionView() = default;

		explicit MemoryRegionView(T* ptr)
			: m_ptr(ptr)
		{
		}

		T* Get()
		{
			return m_ptr;
		}

		const T* Get() const
		{
			return m_ptr;
		}

		T* operator->()
		{
			return m_ptr;
		}

		T const* operator->() const
		{
			return m_ptr;
		}

	private:
		T* m_ptr = nullptr;
	};

	class MemoryRegion
	{
	public:
		MemoryRegion() = default;

		explicit MemoryRegion(std::unique_ptr<byte[]> buffer, size_t size)
			: m_buffer(std::move(buffer))
			, m_size(size)
		{
		}

		byte* Data()
		{
			return m_buffer.get();
		}

		size_t Size() const { return m_size; }

		template<typename T>
		MemoryRegionView<T> GetView(size_t offset = 0)
		{
			return MemoryRegionView(reinterpret_cast<T*>(m_buffer.get() + offset));
		}

	private:
		std::unique_ptr<byte[]> m_buffer;
		size_t m_size = 0ull;
	};
}