#pragma once

#include <string>
#include <stdint.h>
#include <cstddef>

namespace phx
{
	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);

		operator uint64_t() const { return this->m_uuid; }

		std::string ToString() const
		{
			const uint32_t a = (m_uuid >> 32) & 0xFFFFFFFF;
			const uint16_t b = (m_uuid >> 16) & 0xFFFF;
			const uint16_t c = (m_uuid) & 0xFFFF;

			char buf[37]; // 32 hex + 4 dashes + null
			std::snprintf(buf, sizeof(buf),
						  "%08x-0000-0000-%04x-%04x00000000",
						  a, b, c);

			return std::string(buf);
		}

		static UUID FromString(const std::string &str)
		{
			std::string hex;
			hex.reserve(16);
			for (char c : str)
				if (c != '-')
					hex += c;
					
			const uint64_t a = std::stoull(hex.substr(0, 8), nullptr, 16);
			const uint64_t b = std::stoull(hex.substr(8, 4), nullptr, 16);
			const uint64_t c = std::stoull(hex.substr(16, 4), nullptr, 16);

			const uint64_t uuid = (a << 32) | (b << 16) | c;
			return UUID(uuid);
		}

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