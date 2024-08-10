#pragma once

#include <stdint.h>
#include <cstddef>
namespace phx
{
	class UUID final
	{
	public:
		UUID();
		UUID(uint64_t uuid)
			: m_uuid(uuid) {};
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
	struct hash<phx::UUID>
	{
		std::size_t operator()(const phx::UUID& uuid) const
		{
			return (uint64_t)uuid;
		}
	};

}