#pragma once

#include <mutex>
#include <PhxEngine/Core/Vector.h>

namespace PhxEngine::Core
{
	// This is a lot like the descriptor pool. Consider reusing that.
	class BitSetAllocator
	{
	public:
		explicit BitSetAllocator(size_t capacity);

		int Allocate();
		void Release(int index);

	private:
		int m_nextAvailable;
		Phx::FlexArray<bool> m_allocated;
		std::mutex m_mutex;

	};
}

