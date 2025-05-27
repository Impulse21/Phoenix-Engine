#pragma once

// Credit to: https://github.dev/FireFlyForLife/phx::dataReflection

#include <PhxCore/Span.h>
#include <PhxCore/Assert.h>
#include <PhxCore/StringHash.h>

#include "ReflectionMacros.h"
#include "Any.h"
#include "TemplatedTypeId.h"

#include <string>

// Credit to: https://github.dev/FireFlyForLife/NeatReflection

// Forward Declarations
namespace phx::data
{
	struct TypeDescriptor;
	struct FieldDescriptor;
}

namespace phx::data
{
	namespace TypeRegistry
	{
		TypeDescriptor& AddType(TypeDescriptor&&);

#if false
		phx::Span<const TypeDescriptor> GetTypes();
#endif
		const TypeDescriptor* GetType(std::string_view type_name);
		const TypeDescriptor* GetType(TemplateTypeId type_id);
		template<typename T>
		const TypeDescriptor* GetType() { return get_type(GetId<T>()); }
	}

	struct BaseClassDescriptor
	{
		// Data
		TemplateTypeId Id = kEmptyTypeId;

		// Operators
		auto operator<=>(const BaseClassDescriptor& other) const noexcept = default;
	};

	struct FieldDescriptor
	{
		TemplateTypeId ObjectType;
		TemplateTypeId Type;
		std::string Name;
		std::vector<std::tuple<std::string, std::string>> Attributes; // Unused currently

		// Functions
		template<typename TObject, typename TType, TType TObject::* PtrToMember>
		static FieldDescriptor Create(std::string_view name);


		using GetValueFunction = Any(*)(AnyPtr object);
		using SetValueFunction = void (*)(AnyPtr object, Any value);
		using GetAddressFunction = AnyPtr(*)(AnyPtr object);
		GetValueFunction GetValue;
		SetValueFunction SetValue;
		GetAddressFunction GetAddress;


		// Operators
		bool operator==(const FieldDescriptor& other) const noexcept;
		std::strong_ordering operator<=>(const FieldDescriptor& other) const noexcept;
	};

	struct TypeDescriptor
	{
		// Data
		std::string Name;
		TemplateTypeId Id;
		size_t Size;
		BaseClassDescriptor Base;
		std::vector<FieldDescriptor> Fields;


		using DefaultConstructorPtr = void (*)(AnyPtr uninitialisedObject);
		using DestructorPtr = void (*)(AnyPtr object);
		DefaultConstructorPtr DefaultConstructor = nullptr;
		DestructorPtr Destructor = nullptr;
		
		TypeDescriptor& AddField(FieldDescriptor&& field)
		{
			Fields.emplace_back(std::forward<FieldDescriptor>(field));
			return *this;
		};

		// Functions
		template<typename T>
		static TypeDescriptor Create(
			std::string_view name);

		// Operators
		bool operator==(const TypeDescriptor& other) const noexcept;
		std::strong_ordering operator<=>(const TypeDescriptor& other) const noexcept;
	};

}

namespace phx::data
{
	namespace detail
	{
		template<typename T>
		void DefaultConstructorErased(AnyPtr uninitialisedObject)
		{
			PHX_CORE_ASSERT(uninitialisedObject.TypeId == GetId<T>())

			new (uninitialisedObject.ValuePtr) T{};
		}

		template<typename T>
		void DestructorErased(AnyPtr object)
		{
			PHX_CORE_ASSERT(object.TypeId == GetId<T>());

			T* object_ = static_cast<T*>(object.ValuePtr);
			object_->~T();
		}
	}



	template<typename T>
	TypeDescriptor TypeDescriptor::Create(
		std::string_view name)
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
			.Name = std::string(name),
			.Id = GetId<T>(),
			.Size = sizeof(T),
			.Base = {},
			.DefaultConstructor = defaultConstructor,
			.Destructor = destructor
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
		void SetFieldErased(AnyPtr object, Any value)
		{
			assert(object.TypeId == GetId<TObject>());
			assert(value.TypeId() == GetId<TType>());

			TObject* object_ = static_cast<TObject*>(object.ValuePtr);
			object_->*PtrToMember = value.Value<TType>();
		}

		template<typename TObject, typename TType, TType TObject::* PtrToMember>
		AnyPtr GetFieldAddressErased(AnyPtr object)
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
		SetValueFunction setValue = nullptr;
		if constexpr (std::is_assignable_v<TType&, TType>) 
		{
			setValue = &detail::SetFieldErased<TObject, TType, PtrToMember>;
		}

		return FieldDescriptor{
			.ObjectType = GetId<TObject>(),
			.Type = GetId<TType>(),
			.Name = std::string{ name },
			.Attributes = {},
			.GetValue = &detail::GetFieldErased<TObject, TType, PtrToMember>,
			.SetValue = setValue,
			.GetAddress = &detail::GetFieldAddressErased<TObject, TType, PtrToMember>,
		};
	}


	inline bool TypeDescriptor::operator==(const TypeDescriptor& other) const noexcept
	{
		return (*this <=> other) == std::strong_ordering::equal;
	}

	inline std::strong_ordering TypeDescriptor::operator<=>(const TypeDescriptor& other) const noexcept
	{
		return Id <=> other.Id;
	}

	inline bool FieldDescriptor::operator==(const FieldDescriptor& other) const noexcept
	{
		return (*this <=> other) == std::strong_ordering::equal;
	}

	inline std::strong_ordering FieldDescriptor::operator<=>(const FieldDescriptor& other) const noexcept
	{
		std::strong_ordering order;

		order = (ObjectType <=> other.ObjectType);
		if (order != 0) 
			return order;

		order = (Name <=> other.Name);

		return order;
	}

}


namespace phx::HashUtils
{

	template <typename T, typename... Rest>
	void Combine(std::size_t& seed, const T& v, const Rest&... rest)
	{
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		(Combine(seed, rest), ...);
	}
}
namespace std
{
	template<typename T>
	struct hash;

	template<>
	struct hash<phx::data::TypeDescriptor>
	{
		size_t operator()(const phx::data::TypeDescriptor& type) const noexcept
		{
			return type.Id;
		}
	};

	template<>
	struct hash<phx::data::FieldDescriptor>
	{
		size_t operator()(const phx::data::FieldDescriptor& field) const noexcept
		{
			size_t h = 0;
			phx::HashUtils::Combine(h, field.ObjectType, field.Name);
			return h;
		}
	};

	template<>
	struct hash<phx::data::BaseClassDescriptor>
	{
		size_t operator()(const phx::data::BaseClassDescriptor& baseClass) const noexcept
		{
			return baseClass.Id;
		}
	};
}