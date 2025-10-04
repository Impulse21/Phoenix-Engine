#include <PhxData/PhxData_pch.h>
#include "Any.h"

#include <type_traits>
#include <cassert>

// Credit to: https://github.dev/FireFlyForLife/NeatReflection

namespace phx::data
{
	Any::Any(const Any& other) noexcept
	{
		// Assign other storage
		if (other.m_storage_mode == StorageMode::InlineValue) 
		{
			memcpy(m_storage.InlineValue, other.m_storage.InlineValue, Storage::kInlineStorageSize);
		}
		else if (other.m_storage_mode == StorageMode::BoxedValue) 
		{
			new (&m_storage.BoxedValue) std::shared_ptr<void>{ other.m_storage.BoxedValue };
		}

		// Assign other storage
		m_tempalate_type_id = other.m_tempalate_type_id;
		m_storage_mode = other.m_storage_mode;
	}

	Any::Any(Any&& other) noexcept
	{
		// Assign other storage
		if (other.m_storage_mode == StorageMode::InlineValue) 
		{
			memcpy(m_storage.InlineValue, other.m_storage.InlineValue, Storage::kInlineStorageSize);
		}
		else if (other.m_storage_mode == StorageMode::BoxedValue) 
		{
			new (&m_storage.BoxedValue) std::shared_ptr<void>{ std::move(other.m_storage.BoxedValue) };
		}

		// Assign other storage
		m_tempalate_type_id = other.m_tempalate_type_id;
		m_storage_mode = other.m_storage_mode;

		// Clear other
		other.m_tempalate_type_id = kEmptyTypeId;
		other.m_storage_mode = StorageMode::Empty;
	}

	Any& Any::operator=(const Any& other) noexcept
	{
		// Self assignment check
		if (&other == this) 
		{
			return *this;
		}

		// Destroy this
		this->~Any();

		// Assign other
		new (this) Any{ other };

		return *this;
	}

	Any& Any::operator=(Any&& other) noexcept
	{
		// Self assignment check
		if (&other == this) 
		{
			return *this;
		}

		// Destroy this
		this->~Any();

		// Move assign other
		new (this) Any{ std::move(other) };

		return *this;
	}

	Any::~Any()
	{
		// Destroy storage
		if (m_storage_mode == StorageMode::BoxedValue) 
		{
			m_storage.BoxedValue.~shared_ptr();
		}
		else if (m_storage_mode == StorageMode::InlineValue) 
		{
			// SBO is limited to trivial type currently, so no destruction needs to happen
		}
	}

	bool Any::HasValue() const
	{
		return m_storage_mode != StorageMode::Empty;
	}

	TemplateTypeId Any::TypeId() const
	{
		return m_tempalate_type_id;
	}

	AnyPtr Any::ToAnyPtr()
	{
		if (!HasValue()) 
		{
			return AnyPtr{};
		}

		return AnyPtr{ .value_ptr = ObjectPtr(), .type_id = m_tempalate_type_id };
	}

	void* Any::ObjectPtr()
	{
		switch (m_storage_mode)
		{
		case StorageMode::InlineValue: return m_storage.InlineValue;
		case StorageMode::BoxedValue: return m_storage.BoxedValue.get();
		case StorageMode::Empty: return nullptr;
		}

		PHX_CORE_ASSERT(false && "Unexpected AnyStorageType flag.");
		return nullptr;
	}
}
