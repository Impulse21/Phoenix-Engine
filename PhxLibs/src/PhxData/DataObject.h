#pragma once

#include <PhxCore/StringHash.h>
#include "Reflection.h"

#define PHX_DATA_OBJECT(typeName, baseTypeName)                                                                 \
        using ClassName = typeName;                                                                             \
        using BaseClassName = baseTypeName;                                                                     \
                                                                                                                \
        inline static constexpr phx::StringHash TypeId = StringHash(#typeName);                                 \
        inline static constexpr const char* TypeName = #typeName;                                               \
                                                                                                                \
        static const phx::data::TypeInfo& GetTypeInfoStatic() { return Reflection<ClassName>::GetTypeInfo(); }  \
                                                                                                                \
        virtual const phx::data::TypeInfo& GetTypeInfo() const override { return GetTypeInfoStatic(); }         \
        virtual phx::StringHash GetType() const override { return TypeId; }                                     \
        virtual const char* GetTypeName() const override { return TypeName; }                                   \

#define PROPERTY(...)


namespace phx::data
{
    struct IDataObj
    {
        virtual phx::StringHash GetType() const = 0;
        virtual const char* GetTypeName() const = 0;
        virtual const TypeInfo& GetTypeInfo() const = 0;

    public:
        virtual unsigned long AddRef() = 0;
        virtual unsigned long Release() = 0;
    };
}
