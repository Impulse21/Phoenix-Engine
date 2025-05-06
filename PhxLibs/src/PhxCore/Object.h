#pragma once

#include <string>
#include <PhxCore\Base.h>
#include <PhxCore/StringHash.h>
#include <PhxCore/Span.h>

namespace phx
{
    struct FieldInfo
    {
        std::string_view name;
        size_t offset;
        size_t size;
    };

    class TypeInfo
    {
    public:
        TypeInfo(const char* typeName, const TypeInfo* baseTypeInfo, phx::Span<FieldInfo> fields);
        ~TypeInfo() = default;

        bool IsTypeOf(StringHash type) const;
        bool IsTypeOf(const TypeInfo* typeInfo) const;

        template<typename T> 
        bool IsTypeOf() const { return IsTypeOf(GetTypeInfoStatic<T>()); }

        StringHash GetType() const { return m_type; }
        const std::string& GetTypeName() const { return m_typeName; }
        const TypeInfo* GetBaseTypeInfo() const { return m_baseTypeInfo; }

        phx::Span<FieldInfo> GetFields() const { return m_fields; }

    private:
        /// Type.
        StringHash m_type;
        /// Type name.
        std::string m_typeName;
        /// Base class type info.
        const TypeInfo* m_baseTypeInfo;

        phx::Span<FieldInfo> m_fields;
    };


    template<typename T>
    const TypeInfo* GetStaticTypeInfo()
    {
        static_assert(sizeof(T) == 0, "Reflection not implemented for this type.");
        return nullptr;
    }


	class Object
	{
	public:
        Object() = default;
        virtual ~Object() = default;

        virtual StringHash GetType() const = 0;
        virtual const std::string& GetTypeName() const = 0;
        virtual const TypeInfo* GetTypeInfo() const = 0;

        static const TypeInfo* GetTypeInfoStatic() { return nullptr; }


        bool IsInstanceOf(StringHash type) const { return GetTypeInfo()->IsTypeOf(type); }
        bool IsInstanceOf(const TypeInfo* typeInfo) const { return GetTypeInfo()->IsTypeOf(typeInfo); }
        template<typename T> 
        bool IsInstanceOf() const { return IsInstanceOf(T::GetTypeInfoStatic()); }

        template<typename T> 
        T* As() { return IsInstanceOf<T>() ? static_cast<T*>(this) : nullptr; }

        template<typename T> 
        const T* As() const { return IsInstanceOf<T>() ? static_cast<const T*>(this) : nullptr; }
	};
}