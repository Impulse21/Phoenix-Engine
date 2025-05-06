#pragma once

#include "TypeInfo.h"
#include <PhxCore/Base.h>

#define REFLECT_BEGIN(typeName)						\
template<>                                          \
struct phx::data::TypeReflection<typeName>          \
{													\
	using ClassName = typeName;						\
    static consteval const TypeInfo* Get()          \
    {                                               \
        static constexpr PropertyInfo fields[] = {

#if false
#define REFLECT_FIELD(typeName, fieldName) { #fieldName, phx_offsetof(&typeName::fieldName), sizeof(((typeName*)0)->fieldName) }
#else
#define REFLECT_FIELD(typeName, fieldName)
#endif

#define REFLECT_END(typeName) \
        };                                        \
        static constexpr TypeInfo typeInfo = {   \
            #typeName,                               \
            sizeof(ClassName),                        \
            fields,                               \
            sizeof(fields) / sizeof(fields[0])     \
        };                                        \
        return &typeInfo;                        \
    }                                             \

namespace phx::data
{
	template<typename T>
	concept ReflectableStruct =
		std::is_standard_layout_v<T> &&
		std::is_trivially_copyable_v<T>;

	template<typename T>
		requires ReflectableStruct<T>
	struct TypeReflection
	{
		static consteval const TypeInfo* Get()
		{
			static_assert(sizeof(T) == 0, "You must specialize TypeReflection<T>.");
			return nullptr;
		}
	};

	template<typename T>
	concept Reflectable =
		ReflectableStruct<T> &&
		requires { TypeReflection<T>::Get(); };

	template<typename T>
		requires Reflectable<T>
	consteval const TypeInfo* GetStaticTypeInfo()
	{
		return TypeReflection<T>::Get();
	}
}