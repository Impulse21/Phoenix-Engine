#pragma once

#if false
#include <PhxCore/StringHash.h>
#include "Reflection.h"

#define PHX_DATA_OBJECT(typeName, baseTypeName)                                                                 \
        using ClassName = typeName;                                                                             \
        using BaseClassName = baseTypeName;                                                                     \
                                                                                                                \
        inline static constexpr phx::StringHash TypeId = StringHash(#typeName);                                 \
        inline static constexpr const char* TypeName = #typeName;                                               \
        inline static constexpr phx::StringHash BaseTypeId = StringHash(#baseTypeName);                         \
        inline static constexpr const char* BaseTypeName = #baseTypeName;                                       \
                                                                                                                \
        static const phx::data::TypeInfo& GetTypeInfoStatic();                                                  \
        static phx::StringHash GetTypeIdStatic();                                                               \
        static const phx::data::TypeInfo& GetBaseTypeInfoStatic();                                              \
        static phx::StringHash GetBaseTypeIdStatic();                                                           \
        virtual const phx::data::TypeInfo& GetTypeInfo() const override { return GetTypeInfoStatic(); }         \
        virtual phx::StringHash GetType() const override { return TypeId; }                                     \
        virtual const char* GetTypeName() const override { return TypeName; }                                   \

#define PROPERTY(...)


namespace phx
{
    class IFileSystem;
}

namespace phx::data
{
    class IArchiver;
    struct IDataObj
    {
        virtual phx::StringHash GetType() const = 0;
        virtual const char* GetTypeName() const = 0;
        virtual const TypeInfo& GetTypeInfo() const = 0;

        virtual ~IDataObj() = default;

        virtual void Serialize(IArchiver& ar) const;
        virtual void Deserialize(IArchiver& ar);

        static const phx::data::TypeInfo& GetTypeInfoStatic() { static TypeInfo typeInfo; return typeInfo; }
        static phx::StringHash GetTypeIdStatic() { static StringHash id = "IDataObj"_hash; return id; }

    public:
        virtual unsigned long AddRef() = 0;
        virtual unsigned long Release() = 0;
    };

    void Save(phx::IFileSystem* fs, const char* filename, IDataObj const& dataObj);
    void Load(phx::IFileSystem* fs, const char* filename, IDataObj& dataObj);

}

#endif