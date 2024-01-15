#pragma once

#include <stdint.h>
#include <string_view>

namespace PhxEngine::Core
{
	// FNV-1a 32bit hashing algorithm.
	constexpr uint32_t fnv1a_32(char const* s, size_t count)
	{
		return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
	}

	constexpr size_t const_strlen(const char* s)
	{
		size_t size = 0;
		while (s[size]) { size++; };
		return size;
	}

	using Hash32 = uint32_t;

	class StringHash
	{
	public:
		constexpr StringHash(uint32_t hash) noexcept 
			: m_computedHash(hash)
		{}

		constexpr StringHash(const char* s) noexcept : m_computedHash(0)
		{
			this->m_computedHash = Calculate(s, const_strlen(s));
		}

		constexpr StringHash(const char* s, std::size_t count) noexcept 
			: m_computedHash(0)
		{
			this->m_computedHash = Calculate(s, count);
		}

		constexpr StringHash(std::string_view s) noexcept
			: m_computedHash(0)
		{
			this->m_computedHash = Calculate(s.data(), s.size());
		}

		StringHash(const StringHash& other) = default;

		StringHash()
			: m_computedHash(0) {};

		constexpr operator Hash32()noexcept { return this->m_computedHash; }
		explicit operator bool() const { return this->m_computedHash != 0; }
		Hash32 Value() const { return this->m_computedHash; }
		Hash32 ToHash() const { return this->m_computedHash; }

		static constexpr Hash32 Calculate(const char* str, size_t count)
		{
			return Calculate(str, count);
		}

	private:
		Hash32 m_computedHash;

	};

	constexpr StringHash operator ""_hash(const char* s, size_t)
	{
		return StringHash(s, const_strlen(s));
	}

}

namespace std 
{
	template <>
	struct hash<PhxEngine::Core::StringHash> 
	{
		size_t operator()(PhxEngine::Core::StringHash const& obj) const
		{
			return static_cast<size_t>(obj.ToHash());
		}
	};
}