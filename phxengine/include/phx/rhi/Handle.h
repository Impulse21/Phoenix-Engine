#pragma once

#include <stdint.h>

namespace phx::rhi
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
		Handle(uint16_t index, uint16_t generation)
			: m_index(index)
			, m_generation(generation)
		{};

	private:
		uint16_t m_index;
		uint16_t m_generation;

		template<class THandle, class THotData, class TColdData>
		friend class ResourcePool;
	};
	
	static_assert(sizeof(Handle<uint16_t>) == sizeof(uint32_t));
}