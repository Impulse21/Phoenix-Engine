#pragma once

#include <cstring>
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

		inline static MemoryBuffer CreateCopy(const MemoryBuffer& other) 
		{
			std::unique_ptr<std::byte[]> new_buffer = std::make_unique<std::byte[]>(other.Size());
			std::memcpy(new_buffer.get(), other.Data(), other.Size());

			return MemoryBuffer(std::move(new_buffer), other.Size()); 
		}

	public:
		MemoryBuffer() = default;

		explicit MemoryBuffer(size_t size)
			: m_buffer(std::make_unique<byte[]>(size))
			, m_size(size)
		{
		}

		explicit MemoryBuffer(size_t size, std::byte init_value)
			: m_buffer(std::make_unique<byte[]>(size))
			, m_size(size)
		{
			std::fill_n(m_buffer.get(), size, init_value);
		}

		explicit MemoryBuffer(std::unique_ptr<std::byte[]>&& data, size_t size)
			: m_buffer(std::move(data))
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

		const byte* Data() const
		{
			return m_buffer.get();
		}

		size_t Size() const { return m_size; }
		bool IsEmpty() const { return m_size == 0; }

		template<TrivalType T>
		TypedView<T> GetView(size_t offset = 0)
		{
			return TypedView(reinterpret_cast<T*>(m_buffer.get() + offset));
		}

	private:
		std::unique_ptr<byte[]> m_buffer;
		size_t m_size = 0ull;
	};
}