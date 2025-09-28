#pragma once

#include "Reflection.h"
#include "TypeInfo.h"

namespace phx::reflection
{
	namespace detail
	{
		// Forward declaration for GetFieldErased
		template<typename TObject, typename TType, TType TObject::* PtrToMember>
		Any GetFieldErased(AnyPtr object);

		// Forward declaration for SetFieldErased
		template<typename TObject, typename TType, TType TObject::* PtrToMember>
		void SetFieldErased(AnyPtr object, Any value);

		// Forward declaration for GetFieldAddressErased
		template<typename TObject, typename TType, TType TObject::* PtrToMember>
		AnyPtr GetFieldAddressErased(AnyPtr object);
	}

	template<typename T>
	class ReflectionBuilder
	{
	public:
		ReflectionBuilder(const char* name) 
        {
#if false
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
#endif
			m_typeInfo = &T::s_typeInfo;
			m_typeInfo->name = name;
			m_typeInfo->size = sizeof(T);
		}
		

        template<typename ParentT>
        ReflectionBuilder& Parent()
        {
            m_typeInfo->parent = ParentT::GetStaticTypeInfo();
            return *this;
        }

		template<typename TType, TType T::* PtrToMember>
        ReflectionBuilder& Property(const char* name, std::initializer_list<PropertyInfo> props = {})
        {
			MemberInfo::SetValueFunction set_value = nullptr;
			if constexpr (std::is_assignable_v<TType&, TType>)
			{
				set_value = &detail::SetFieldErased<T, TType, PtrToMember>;
			}
			MemberInfo info = {
				.type = GetId<TType>(),
				.object_type = GetId<T>(),
				.name = name,
				.properties = {},
				.get_value = &detail::GetFieldErased<T, TType, PtrToMember>,
				.set_value = set_value,
				.get_address = &detail::GetFieldAddressErased<T, TType, PtrToMember>,
			};

			m_typeInfo->members.push_back(info);
			return *this;
        }

		void Register()
		{
			IReflectionRegistry::Ptr->RegisterType(m_typeInfo);
		}

	private:
		TypeInfo* m_typeInfo;
	};

	namespace detail
	{
		template<typename TObject, typename TType, TType TObject::* PtrToMember>
		Any GetFieldErased(AnyPtr object)
		{
			assert(object.type_id == GetId<TObject>());

			TObject* object_ = static_cast<TObject*>(object.value_ptr);
			return object_->*PtrToMember;
		}

		template<typename TObject, typename TType, TType TObject::* PtrToMember>
		void SetFieldErased(AnyPtr object, Any value)
		{
			assert(object.type_id == GetId<TObject>());
			assert(value.TypeId() == GetId<TType>());

			TObject* object_ = static_cast<TObject*>(object.value_ptr);
			object_->*PtrToMember = value.Value<TType>();
		}

		template<typename TObject, typename TType, TType TObject::* PtrToMember>
		AnyPtr GetFieldAddressErased(AnyPtr object)
		{
			assert(object.type_id == GetId<TObject>());

			TObject* object_ = static_cast<TObject*>(object.value_ptr);

			TType* address = &(object_->*PtrToMember);
			return AnyPtr{ (void*)address, GetId<TType>() };
		}
	}
}