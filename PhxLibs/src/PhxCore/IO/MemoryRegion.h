#pragma once

#include <memory>
#include <type_traits>

namespace phx
{
	template<typename T>
	concept TrivalType = std::is_trivial_v<T>;

	using byte = std::byte;

	template<TrivalType T>
	class TypedView
	{
	public:
		TypedView() = default;

		explicit TypedView(T* ptr)
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

	class MemoryBuffer
	{
	public:
		template<typename T>
		inline static MemoryBuffer Create() { return MemoryBuffer(sizeof(T)); }
	public:
		MemoryBuffer() = default;

		explicit MemoryBuffer(size_t size)
			: m_buffer(std::make_unique<byte[]>(size))
			, m_size(size)
		{
		}
		
		MemoryBuffer(const MemoryBuffer&) = delete;
		MemoryBuffer& operator=(const MemoryBuffer&) = delete;

		MemoryBuffer(MemoryBuffer&&) = default;
		MemoryBuffer& operator=(MemoryBuffer&&) = default;

		~MemoryBuffer() = default;

		byte* Data()
		{
			return m_buffer.get();
		}

		size_t Size() const { return m_size; }

		template<TrivalType T>
		TypedView<T> GetView(size_t offset = 0)
		{
			return TypedView(reinterpret_cast<T*>(m_buffer.get() + offset));
		}

	private:
		std::unique_ptr<byte[]> m_buffer;
		size_t m_size = 0ull;
	};


	// DEPERICATED
	template<typename T>
	class MemoryRegion
	{
	public:
		MemoryRegion() = default;

		MemoryRegion(std::unique_ptr<char[]> buffer)
			: m_buffer(std::move(buffer))
		{
		}

		char* Data()
		{
			return m_buffer.get();
		}

		T* Get()
		{
			return reinterpret_cast<T*>(m_buffer.get());
		}

		const T* Get() const
		{
			return reinterpret_cast<T*>(m_buffer.get());
		}

		T* operator->()
		{
			return reinterpret_cast<T*>(m_buffer.get());
		}

		T const* operator->() const
		{
			return reinterpret_cast<T const*>(m_buffer.get());
		}

	private:
		std::unique_ptr<char[]> m_buffer;

	};

}