#pragma once

#include "TemplatedTypeId.h"
#include <type_traits>
#include <memory>

// Credit to: https://github.dev/FireFlyForLife/NeatReflection
namespace phx::data
{
	class Any;

	struct AnyPtr
    {
        // Data
        void* ValuePtr = nullptr;
        TemplateTypeId TypeId = kEmptyTypeId;

        // Operators
        auto operator<=>(const AnyPtr& other) const noexcept = default;
    };

	template<typename T>
	concept NotAny = !std::is_same_v<std::decay_t<T>, Any>;

	class Any
	{
	public:
		// Construction & Deconstruction
		Any() = default;
		template<NotAny T>
		Any(T&& value);
		Any(const Any& other) noexcept;
		Any(Any&& other) noexcept;

		Any& operator=(const Any& other) noexcept;
		Any& operator=(Any&& other) noexcept;
		template<NotAny T>
		Any& operator=(T&& value);

		~Any();

		// Accessors
		bool HasValue() const;
		TemplateTypeId TypeId() const;
		template<typename T>
		T& Value();
		template<typename T>
		T* ValuePtr();

		// Conversion
		AnyPtr ToAnyPtr();

	private:
		// Helpers
		void* ObjectPtr();

		// Private data types
		enum class StorageMode : uint8_t { Empty, InlineValue, BoxedValue };

		union alignas(max_align_t) Storage
		{
			static constexpr size_t kInlineStorageSize = 24;

			~Storage() {} // Destruction handled in Any

			std::shared_ptr<void> BoxedValue{}; // Active when `storage_mode == StorageMode::BoxedValue`
			std::byte InlineValue[kInlineStorageSize]; // Active when `storage_mode == StorageMode::InlineValue`
		};

		// Data
		Storage m_storage;
		TemplateTypeId m_tempalateTypeId = kEmptyTypeId;
		StorageMode m_storageMode = StorageMode::Empty;
	};
}


// Implementation
namespace phx::data
{
	template<NotAny T>
	Any::Any(T&& value)
	{
		using CleanT = std::remove_cvref_t<T>;

		m_tempalateTypeId = GetId<CleanT>();

		if (sizeof(CleanT) <= Storage::kInlineStorageSize && alignof(CleanT) <= alignof(Storage) && std::is_trivial_v<CleanT>) 
		{
			new (m_storage.InlineValue) CleanT{ value };
			m_storageMode = StorageMode::InlineValue;
		}
		else {
			new (&m_storage.BoxedValue) std::shared_ptr<void>{ std::make_shared<CleanT>(value) };
			m_storageMode = StorageMode::BoxedValue;
		}
	}

	template<NotAny T>
	Any& Any::operator=(T&& value)
	{
		// Destruct this
		this->~Any();

		// Call into this value constructor
		new (this) Any{ std::forward<T>(value) };

		return *this;
	}

	template<typename T>
	T& Any::Value()
	{
		assert(HasValue());
		assert(GetId<T>() == m_tempalateTypeId);
		return *static_cast<T*>(ObjectPtr());
	}

	template<typename T>
	T* Any::ValuePtr()
	{
		if (GetId<T>() != m_tempalateTypeId) 
		{
			return nullptr;
		}

		return static_cast<T*>(ObjectPtr());
	}
}
