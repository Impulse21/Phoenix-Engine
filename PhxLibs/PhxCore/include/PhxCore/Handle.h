#pragma once

#include <stdint.h>

#define PHX_DEFINE_OPAQUE_HANDLE(name) struct name##_T; using name = name##_T*;

namespace phx
{
	// Index based handle
	template<typename T>
	class Handle
	{
	public:
		Handle()
			: m_index(0)
			, m_generation(0)
		{}

		Handle(uint16_t index, uint16_t generation)
			: m_index(index)
			, m_generation(generation)
		{
		}

		bool IsValid() const { return this->m_generation != 0; }

		bool operator==(const Handle& rhs) const
		{
			return this->m_generation == rhs.m_generation && this->m_index == rhs.m_index;
		}

	private:
		uint16_t m_index;
		uint16_t m_generation;

		template<class THandle, class THotData, class TColdData>
		friend class PagedPool;
		friend struct std::hash<Handle<T>>;
	};
	
	static_assert(sizeof(Handle<uint16_t>) == sizeof(uint32_t));
}

namespace std
{
	template<typename T> // This must also be a template
	struct hash<phx::Handle<T>>
	{
		size_t operator()(const phx::Handle<T>& handle) const noexcept
		{
			const uint16_t index = handle.m_index;
			const uint16_t generation = handle.m_generation;
			uint32_t combined_value = (static_cast<uint32_t>(generation) << 16) | static_cast<uint32_t>(index);
			return std::hash<uint32_t>{}(combined_value);
		}
	};
} // namespace std
