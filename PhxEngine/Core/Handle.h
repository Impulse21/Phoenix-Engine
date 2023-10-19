#pragma once

#include <stdint.h>

namespace PhxEngine::Core
{
	template<typename T>
	class Handle
	{
	public:
		Handle()
			: m_index(0)
			, m_generation(0)
		{}

		bool IsValid() const { return this->m_generation != 0; }

		bool operator==(const Handle& rhs) const
		{
			return this->m_generation == rhs.m_generation && this->m_index == rhs.m_index;
		}
	private:
		Handle(uint32_t index, uint32_t generation)
			: m_index(index)
			, m_generation(generation)
		{};

	private:
		uint32_t m_index;
		uint32_t m_generation;

		template<typename ImplT, typename HT>
		friend class Pool;

	};
}