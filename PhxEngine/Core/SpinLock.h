#pragma once

#include <atomic>

namespace PhxEngine::Core
{
	class SpinLock
	{
	public:
		SpinLock()
		{
			this->m_flag.clear();
		}

		void Lock()
		{
			while (this->m_flag.test_and_set(std::memory_order_acquire))
			{
				// Spin until we acquire a lock
			}
		}

		void Unlock()
		{
			this->m_flag.clear(std::memory_order_release);
		}

	private:
		std::atomic_flag m_flag;
	};

	class ScopedSpinLock
	{
	public:
		ScopedSpinLock(SpinLock& lock)
			: m_lock(lock)
		{
			this->m_lock.Lock();
		}

		~ScopedSpinLock()
		{
			this->m_lock.Unlock();
		}

	private:
		SpinLock& m_lock;
	};
}