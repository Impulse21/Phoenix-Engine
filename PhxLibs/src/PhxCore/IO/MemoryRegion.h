#pragma once

#include <memory>

namespace phx
{

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