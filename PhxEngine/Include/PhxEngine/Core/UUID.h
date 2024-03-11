#pragma once

#include <stdint.h>
#include <cstddef>

namespace PhxEngine
{

	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		operator uint64_t() const { return this->m_uuid; }
	private:
		uint64_t m_uuid;
	};

}

namespace std 
{
	template <typename T> struct hash;

	template<>
	struct hash<PhxEngine::UUID>
	{
		std::size_t operator()(const PhxEngine::UUID& uuid) const
		{
			return (uint64_t)uuid;
		}
	};

}