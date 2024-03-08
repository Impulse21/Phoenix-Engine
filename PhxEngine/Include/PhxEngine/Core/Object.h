#pragma once

#include <PhxEngine/Core/StringHash.h>
#include <unordered_map>
#include <variant>
#include <memory>
#include <functional>

namespace PhxEngine
{
    using VariantMap = std::unordered_map<StringHash, std::variant<int, size_t, float, std::string>>;

	struct ObjectId;


    /// Type info.
    /// @nobind
    class TypeInfo
    {
    public:
        /// Construct.
        TypeInfo(const char* typeName, const TypeInfo* baseTypeInfo);
        ~TypeInfo() = default;

        bool IsTypeOf(StringHash type) const;
        bool IsTypeOf(const TypeInfo* typeInfo) const;
        template<typename T> bool IsTypeOf() const { return IsTypeOf(T::GetTypeInfoStatic()); }

        StringHash GetType() const { return m_type; }
        const std::string& GetTypeName() const { return m_typeName; }
        const TypeInfo* GetBaseTypeInfo() const { return m_baseTypeInfo; }

    private:
        /// Type.
        StringHash m_type;
        /// Type name.
        std::string m_typeName;
        /// Base class type info.
        const TypeInfo* m_baseTypeInfo;
    };


#define PHX_OBJECT(typeName, baseTypeName) \
    public: \
        using ClassName = typeName; \
        using BaseClassName = baseTypeName; \
        virtual PhxEngine::StringHash GetType() const override { return GetTypeInfoStatic()->GetType(); } \
        virtual const std::string& GetTypeName() const override { return GetTypeInfoStatic()->GetTypeName(); } \
        virtual const PhxEngine::TypeInfo* GetTypeInfo() const override { return GetTypeInfoStatic(); } \
        static PhxEngine::StringHash GetTypeStatic() { return GetTypeInfoStatic()->GetType(); } \
        static const std::string& GetTypeNameStatic() { return GetTypeInfoStatic()->GetTypeName(); } \
        static const PhxEngine::TypeInfo* GetTypeInfoStatic() { static const PhxEngine::TypeInfo typeInfoStatic(#typeName, BaseClassName::GetTypeInfoStatic()); return &typeInfoStatic; }

    class EventHandler;
    class Object
    {
    public:
        Object();
        virtual ~Object();

        virtual StringHash GetType() const = 0;
        virtual const std::string& GetTypeName() const = 0;
        virtual const TypeInfo* GetTypeInfo() const = 0;
        virtual void OnEvent(Object* sender, StringHash eventType, VariantMap& eventData);

        /// Return type info static.
        static const TypeInfo* GetTypeInfoStatic() { return nullptr; }
        bool IsInstanceOf(StringHash type) const;
        bool IsInstanceOf(const TypeInfo* typeInfo) const;
        template<typename T> bool IsInstanceOf() const { return IsInstanceOf(T::GetTypeInfoStatic()); }
        template<typename T> T* Cast() { return IsInstanceOf<T>() ? static_cast<T*>(this) : nullptr; }
        template<typename T> const T* Cast() const { return IsInstanceOf<T>() ? static_cast<const T*>(this) : nullptr; }

    };

}