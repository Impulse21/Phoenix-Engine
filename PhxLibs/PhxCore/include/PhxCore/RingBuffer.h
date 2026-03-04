#pragma once

#include <mutex>
namespace phx
{
	template<class TData, size_t BufferCapacity>
	class ThreadSafeRingBuffer
	{    
		static_assert((BufferCapacity & (BufferCapacity - 1)) == 0, "BufferCapacity must be a power of two.");
		static_assert(BufferCapacity > 1);
    	static constexpr size_t BufferMask = BufferCapacity - 1;

	public:
		ThreadSafeRingBuffer() = default;

		ThreadSafeRingBuffer(const ThreadSafeRingBuffer&) = delete;
		ThreadSafeRingBuffer& operator=(const ThreadSafeRingBuffer&) = delete;

	public:
		bool Push(TData const& data)
		{
			std::scoped_lock _(m_lock);

			if (IsFull_UnLocked())
				return false;

			m_data[m_tail] = data;
			m_tail = (m_tail + 1) & BufferMask;

			return true;
		}

		bool Pop(TData& item)
		{
			std::scoped_lock _(m_lock);
			
			if (IsEmpty_UnLocked())
				return false;

			item = std::move(m_data[m_head]);
			m_head = (m_head + 1) & BufferMask;

			return true;
		}

		bool IsEmpty()
		{
			std::scoped_lock _(m_lock);
			return m_tail == m_head;
		}

		bool IsFull()
		{
			std::scoped_lock _(m_lock);
			return ((m_tail + 1) & BufferMask) == m_head;
		}

		size_t Size()
		{
			std::scoped_lock _(m_lock);
			return (m_tail - m_head) & BufferMask;
		}

	private:
    	bool IsEmpty_UnLocked() const { return m_tail == m_head; }
    	bool IsFull_UnLocked()  const { return ((m_tail + 1) & BufferMask) == m_head; }

	private:
		std::mutex m_lock;
		
		size_t m_head = 0;
		size_t m_tail = 0;
		
		TData m_data[BufferCapacity];
	};
}