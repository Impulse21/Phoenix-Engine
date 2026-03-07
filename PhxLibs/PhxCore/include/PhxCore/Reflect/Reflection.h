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
       .kind = phx::reflect::KindOf<decltype(StructType::member)>::value,   \
       .offset = offsetof(StructType, member),                              \
       .size = sizeof(decltype(StructType::member)),                        \
    }


#define PHX_FIELD_NESTED(member, MemberType)                                \
    phx::reflect::FieldInfo{                                                \
       .name = #member,                                                     \
       .kind = phx::reflect::FieldKind::Nested,                             \
       .offset = offsetof(StructType, member),                              \
       .size = sizeof(MemberType),                                          \
       .nested_type = phx::reflect::TypeRegistry::Find<MemberType>(),        \
    }

#define PHX_FIELD_ASSET(member, AssetType)                                  \
    phx::reflect::FieldInfo{                                                \
       .name = #member,                                                     \
       .kind = phx::reflect::FieldKind::AssetPtr,                           \
       .offset = offsetof(StructType, member),                              \
       .size = sizeof(AssetType),                                           \
       .nested_type = phx::reflect::TypeRegistry::Find<AssetType>(),         \
    }



#define PHX_FIELD_ARRAY(member, MemberType)                                 \
    phx::reflect::FieldInfo{                                                \
       .name = #member,                                                     \
       .kind = phx::reflect::FieldKind::Array,                              \
       .offset = offsetof(StructType, member),                              \
       .size = sizeof(decltype(StructType::member)),                        \
       .nested_type = phx::reflect::TypeRegistry::Find<MemberType>(),        \
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
            .construct        = static_cast<void*(*)()>(                                        \
                                    []() -> void* { return new Type(); }),                      \
            .destruct         = static_cast<void(*)(void*)>(                                    \
                                    [](void* p) { delete static_cast<Type*>(p); }),             \
            .construct_place  = static_cast<void(*)(void*)>(                                    \
                                    [](void* p) { new (p) Type(); }),                           \
            .destruct_place   = static_cast<void(*)(void*)>(                                    \
                                    [](void* p) { static_cast<Type*>(p)->~Type(); }),           \
        };                                                                                      \
        phx::reflect::TypeRegistry::RegisterType(info.name, std::move(info));                   \
    };