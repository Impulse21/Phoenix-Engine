#pragma once

#include "Reflection.h"
#include "TypeInfo.h"

namespace phx::reflection
{
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
			T
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

        template<typename MemberType>
        ReflectionBuilder& Property(const char* name, MemberType T::* memberPtr, std::initializer_list<Property> props = {})
        {
            m_typeInfo->members.push_back({
                name,
                GetTypeInfo_Internal<MemberType>(),
                offsetof(T, *(&((T*)0->*memberPtr))),
                std::vector<PropertyInfo>(props)
                });
            return *this;
        }
	private:
		TypeInfo* m_typeInfo;
	};
}