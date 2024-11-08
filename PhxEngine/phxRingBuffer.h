#pragma once

#include <mutex>
namespace phx
{
	template<class TData, size_t BufferCapacity>
	class ThreadSafeRingBuffer
	{
		static_assert((BufferCapacity & (BufferCapacity - 1)) == 0, "BufferCapacity must be a power of two.");
	public:
		bool Push(TData const& data)
		{
			std::scoped_lock _(m_lock);

			if (IsFull())
				return false;

			m_data[m_tail] = data;
			m_tail = (m_tail + 1) & BufferMask;

			return true;
		}

		bool Pop(TData& item)
		{
			std::scoped_lock _(m_lock);
			
			if (IsEmpty())
				return false;

			item = m_data[m_head];

			m_head = m_head + 1 & BufferMask;

			return true;
		}

		bool IsEmpty() const
		{
			return m_tail == m_head;
		}

		bool IsFull() const
		{
			return ((m_tail + 1) & BufferMask) == m_head;
		}

	private:
		const size_t BufferMask = BufferCapacity - 1;
		size_t m_head = 0;
		size_t m_tail = 0;
		TData m_data[BufferCapacity];
		std::mutex m_lock;
	};
}