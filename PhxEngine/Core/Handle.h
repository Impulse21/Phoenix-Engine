#pragma once

#include <stdint.h>

namespace phx
{
	// Index based handle
	template<typename T>
	class Handle
	{
	public:
		static Handle<T> CreateInvalid() { return Handle(); }
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
		Handle(u16 index, u16 generation)
			: m_index(index)
			, m_generation(generation)
		{
		}

	private:

		u16 m_index;
		u16 m_generation;

		template<class THandle, class THotData, class TColdData>
		friend class PagedPool;
		template<class THandle, class TData, u16 MAX_SIZE>
		friend class SmallObjectPool;

#if false
		friend struct std::hash<Handle<T>>;
#endif
	};
	
	static_assert(sizeof(Handle<u16>) == sizeof(uint32_t));
}

#if false
namespace std
{
	template<typename T> // This must also be a template
	struct hash<phx::Handle<T>>
	{
		size_t operator()(const phx::Handle<T>& handle) const noexcept
		{
			const u16 index = handle.m_index;
			const u16 generation = handle.m_generation;
			uint32_t combined_value = (static_cast<uint32_t>(generation) << 16) | static_cast<uint32_t>(index);
			return std::hash<uint32_t>{}(combined_value);
		}
	};
} // namespace std
#endif