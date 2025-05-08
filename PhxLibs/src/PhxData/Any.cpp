#include "PhxData/PhxData_pch.h"
#include "Any.h"

#include <type_traits>
#include <cassert>

// Credit to: https://github.dev/FireFlyForLife/NeatReflection

namespace phx::data
{
	Any::Any(const Any& other) noexcept
	{
		// Assign other storage
		if (other.m_storageMode == StorageMode::InlineValue) 
		{
			memcpy(m_storage.InlineValue, other.m_storage.InlineValue, Storage::kInlineStorageSize);
		}
		else if (other.m_storageMode == StorageMode::BoxedValue) 
		{
			new (&m_storage.BoxedValue) std::shared_ptr<void>{ other.m_storage.BoxedValue };
		}

		// Assign other storage
		m_tempalateTypeId = other.m_tempalateTypeId;
		m_storageMode = other.m_storageMode;
	}

	Any::Any(Any&& other) noexcept
	{
		// Assign other storage
		if (other.m_storageMode == StorageMode::InlineValue) 
		{
			memcpy(m_storage.InlineValue, other.m_storage.InlineValue, Storage::kInlineStorageSize);
		}
		else if (other.m_storageMode == StorageMode::BoxedValue) 
		{
			new (&m_storage.BoxedValue) std::shared_ptr<void>{ std::move(other.m_storage.BoxedValue) };
		}

		// Assign other storage
		m_tempalateTypeId = other.m_tempalateTypeId;
		m_storageMode = other.m_storageMode;

		// Clear other
		other.m_tempalateTypeId = kEmptyTypeId;
		other.m_storageMode = StorageMode::Empty;
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
		if (m_storageMode == StorageMode::BoxedValue) 
		{
			m_storage.BoxedValue.~shared_ptr();
		}
		else if (m_storageMode == StorageMode::InlineValue) 
		{
			// SBO is limited to trivial type currently, so no destruction needs to happen
		}
	}

	bool Any::HasValue() const
	{
		return m_storageMode != StorageMode::Empty;
	}

	TemplateTypeId Any::TypeId() const
	{
		return m_tempalateTypeId;
	}

	AnyPtr Any::ToAnyPtr()
	{
		if (!HasValue()) 
		{
			return AnyPtr{};
		}

		return AnyPtr{ .ValuePtr = ObjectPtr(), .TypeId = m_tempalateTypeId };
	}

	void* Any::ObjectPtr()
	{
		switch (m_storageMode)
		{
		case StorageMode::InlineValue: return m_storage.InlineValue;
		case StorageMode::BoxedValue: return m_storage.BoxedValue.get();
		case StorageMode::Empty: return nullptr;
		}

		PHX_CORE_ASSERT(false && "Unexpected AnyStorageType flag.");
		return nullptr;
	}
}
