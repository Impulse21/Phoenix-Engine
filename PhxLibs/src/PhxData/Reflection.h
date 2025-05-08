#pragma once

#include <PhxCore/Span.h>
#include <PhxCore/Assert.h>
#include "DataPtr.h"

#include "TemplatedTypeId.h"
#include <string>

// Forward Declarations
namespace phx::data
{
	struct TypeDescriptor;
	struct BaseClassDescriptor;
	struct FieldDescriptor;
	struct TemplateArgumentDescriptor;
}

namespace phx::data
{
	struct TypeDescriptor
	{
		// Data
		std::string Name;
		TemplateTypeId Id;
		size_t Size;
		std::vector<BaseClassDescriptor> Bases;
		std::vector<FieldDescriptor> Fields;


		using DefaultConstructorPtr = void (*)(AnyPtr uninitialisedObject);
		using DestructorPtr = void (*)(AnyPtr object);
		DefaultConstructorPtr DefaultConstructor = nullptr;
		DestructorPtr Destructor = nullptr;
		
		// Functions
		template<typename T>
		static TypeDescriptor Create(
			std::string_view name,
			TemplateTypeId id,
			BaseClassDescriptor* base,
			Span<FieldDescriptor> fields);

		// Operators
		bool operator==(const TypeDescriptor& other) const noexcept;
		std::strong_ordering operator<=>(const TypeDescriptor& other) const noexcept;
	};

	struct BaseClassDescriptor
	{
		// Data
		TemplateTypeId BaseId;

		// Operators
		auto operator<=>(const BaseClassDescriptor& other) const noexcept = default;
	};

	struct FieldDescriptor
	{
		TemplateTypeId ObjectType;
		TemplateTypeId Type;
		std::string Name;
		std::vector<std::string> Attributes; // Unused currently

		// Functions
		template<typename TObject, typename TType, TType TObject::* PtrToMember>
		static FieldDescriptor Create(std::string_view name);


		//using GetValueFunction = Any(*)(AnyPtr object);
		//using SetValueFunction = void (*)(AnyPtr object, Any value);
		using GetAddressFunction = AnyPtr(*)(AnyPtr object);
		//GetValueFunction GetValue;
		//SetValueFunction SetValue;
		GetAddressFunction GetAddress;


		// Operators
		bool operator==(const FieldDescriptor& other) const noexcept;
		std::strong_ordering operator<=>(const FieldDescriptor& other) const noexcept;
	};
}

namespace phx::data
{
	namespace detail
	{
		template<typename T>
		void DefaultConstructorErased(AnyPtr uninitialisedObject)
		{
			PHX_ASSERT(uninitialisedObject.TpyeId== GetId<T>());

			new (uninitialisedObject.ValuePtr) T{};
		}

		template<typename T>
		void DestructorErased(AnyPtr object)
		{
			assert(object.TypeId == GetId<T>());

			T* object_ = static_cast<T*>(object.ValuePtr);
			object_->~T();
		}
	}



	template<typename T>
	TypeDescriptor TypeDescriptor::Create(
		std::string_view name,
		TemplateTypeId id,
		BaseClassDescriptor* base,
		phx::Span<FieldDescriptor> fields)
	{
		DefaultConstructorPtr defaultConstructor = nullptr;
		if constexpr (std::is_default_constructible_v<T>) {
			defaultConstructor = &detail::DefaultConstructorErased<T>;
		}

		DestructorPtr destructor = nullptr;
		if constexpr (!std::is_trivially_destructible_v<T>) {
			destructor = &detail::DestructorErased<T>;
		}

		return TypeDescriptor{
			.DefaultConstructor = defaultConstructor,
			.Destructor = destructor,
			.Name = std::string( name ),
			.Id = id,
			.Size = sizeof(T),
			.Base = base,
			.Fields = std::vector(fields.begin(), fields.end()),
		};
	}

	namespace detail
	{
		template<typename TObject, typename TType, TType TObject::* PtrToMember>
		Any GetFieldErased(AnyPtr object)
		{
			assert(object.TypeId == GetId<TObject>());

			TObject* object_ = static_cast<TObject*>(object.ValuePtr);
			return object_->*PtrToMember;
		}

		template<typename TObject, typename TType, TType TObject::* PtrToMember>
		void set_field_erased(AnyPtr object, Any value)
		{
			assert(object.TypeId == GetId<TObject>());
			assert(value.TypeId() == GetId<TType>());

			TObject* object_ = static_cast<TObject*>(object.ValuePtr);
			object_->*PtrToMember = value.value<TType>();
		}

		template<typename TObject, typename TType, TType TObject::* PtrToMember>
		AnyPtr get_field_address_erased(AnyPtr object)
		{
			assert(object.TypeId == GetId<TObject>());

			TObject* object_ = static_cast<TObject*>(object.ValuePtr);

			TType* address = &(object_->*PtrToMember);
			return AnyPtr{ (void*)address, GetId<TType>() };
		}
	}

	template<typename TObject, typename TType, TType TObject::* PtrToMember>
	FieldDescriptor FieldDescriptor::Create(std::string_view name)
	{
		SetValueFunction set_value = nullptr;
		if constexpr (std::is_assignable_v<TType&, TType>) {
			set_value = &Detail::set_field_erased<TObject, TType, PtrToMember>;
		}

		return Field{
			.get_value = &Detail::get_field_erased<TObject, TType, PtrToMember>,
			.set_value = set_value,
			.get_address = &Detail::get_field_address_erased<TObject, TType, PtrToMember>,
			.object_type = GetId<TObject>(),
			.type = GetId<TType>(),
			.name = std::string{ name },
			.access = access
		};
	}
}

