#pragma once

#include <PhxCore/Reflect/TypeInfo.h>

namespace phx::reflect
{

    namespace Detail
    {
        template<class T>
        struct Registrar
        {
            Registrar()
            {
                T::RegisterType();
            }
        };
    }
}


#define PHX_DECLARE_REFLECT(type)                                               \
    static constexpr std::string_view GetTypeNameStatic() { return #type; }     \
    static void RegisterType();                                                 \
    inline static ::phx::reflect::Detail::Registrar<type> _registrar;       


#define PHX_FIELD(member)                                                   \
    phx::reflect::FieldInfo{                                                \
       .name = #member,                                                     \
       .kind = phx::reflect::kindof<decltype(StructType::member)>::value,   \
       .offset = offsetof(StructType, member),                              \
       .size = sizeof(decltype(StructType::member)),                        \
    }


#define PHX_FIELD_NESTED(member, MemberType)                                \
    phx::reflect::FieldInfo{                                                \
       .name = #member,                                                     \
       .kind = phx::reflect::FieldKind::Nested,                             \
       .offset = offsetof(StructType, member),                              \
       .size = sizeof(MemberType),                                          \
       .NestedType = phx::reflect::TypeRegistry::Find<MemberType>(),        \
    }

#define PHX_FIELD_ASSET(member, AssetType)                                  \
    phx::reflect::FieldInfo{                                                \
       .name = #member,                                                     \
       .kind = phx::reflect::FieldKind::AssetRef,                           \
       .offset = offsetof(StructType, member),                              \
       .size = sizeof(AssetType),                                           \
       .NestedType = phx::reflect::TypeRegistry::Find<AssetType>(),         \
    }



#define PHX_FIELD_ARRAY(member, MemberType)                                 \
    phx::reflect::FieldInfo{                                                \
       .name = #member,                                                     \
       .kind = phx::reflect::FieldKind::Array,                              \
       .offset = offsetof(StructType, member),                              \
       .size = sizeof(decltype(StructType::member)),                        \
       .NestedType = phx::reflect::TypeRegistry::Find<MemberType>(),        \
       .element_size = sizeof(MemberType),                                  \
    }

#define PHX_REFLECT(Type, ...)                                                                  \
    inline void Type::RegisterType()                                                            \
    {                                                                                           \
        using StructType = Type;                                                                \
        phx::reflect::TypeInfo info = {                                                         \
            .name = #Type,                                                                      \
            .size = sizeof(Type),                                                               \
            .fields = { __VA_ARGS__ },                                                          \
            .construct = []() { new Type(); },                                                  \
            .destruct = [](void* memory) { delete memory; },                                    \
            .construct_place = [](void* memory) { new (memory) Type(); },                       \
            .destruct_place = [](void* memory) { reinterpret_cast<Type*>(memory)->~Type(); },   \
        };                                                                                      \
        phx::reflect::TypeRegistry::RegisterType(info.name, std::move(info));                   \
    };