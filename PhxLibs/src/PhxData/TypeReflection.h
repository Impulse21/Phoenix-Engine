#pragma once

#include "TypeInfo.h"
#include <PhxCore/Base.h>

#define REFLECT_BEGIN(typeName)									\
	template<>													\
	struct phx::data::TypeReflection<typeName>					\
	{															\
		using ClassName = typeName;								\
		static constexpr FieldInfo fields[] =					\
		{

#define REFLECT_FIELD(typeName, fieldName)


#define REFLECT_END(typeName)									\
        };														\
		static constexpr TypeInfo typeInfo = 					\
		{														\
			.Type = phx::StringHash(#typeName),					\
			.TypeName = #typeName,								\
			.BaseTypeInfo = nullptr,							\
			.Fields = &fields[0],								\
			.NumFields = sizeof(fields) / sizeof(FieldInfo),	\
		};														\
																\
		static consteval const TypeInfo* Get()					\
		{														\
			return &typeInfo;									\
		}														\
	};

namespace phx::data
{
	template<typename T>
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
		requires { TypeReflection<T>::Get(); };

	template<typename T>
		requires Reflectable<T>
	consteval const TypeInfo* GetStaticTypeInfo()
	{
		return TypeReflection<T>::Get();
	}
}