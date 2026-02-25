#pragma once

#include <stdint.h>

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

		bool IsValid() const { return this->m_generation != 0; }

		bool operator==(const Handle& rhs) const
		{
			return this->m_generation == rhs.m_generation && this->m_index == rhs.m_index;
		}

	private:
		Handle(uint16_t index, uint16_t generation)
			: m_index(index)
			, m_generation(generation)
		{
		}

	private:

		uint16_t m_index;
		uint16_t m_generation;

		template<class THandle, class THotData, class TColdData>
		friend class PagedPool;
		template<class THandle, class TData, uint16_t MAX_SIZE>
		friend class SmallObjectPool;
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
