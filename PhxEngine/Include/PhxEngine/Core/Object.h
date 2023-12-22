#pragma once

#include "Handle.h"
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Core/RefCountPtr.h>
#include <PhxEngine/Core/StringHash.h>

namespace PhxEngine::Core
{
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
        virtual PhxEngine::Core::StringHash GetType() const override { return GetTypeInfoStatic()->GetType(); } \
        virtual const std::string& GetTypeName() const override { return GetTypeInfoStatic()->GetTypeName(); } \
        virtual const PhxEngine::Core::TypeInfo* GetTypeInfo() const override { return GetTypeInfoStatic(); } \
        static PhxEngine::Core::StringHash GetTypeStatic() { return GetTypeInfoStatic()->GetType(); } \
        static const std::string& GetTypeNameStatic() { return GetTypeInfoStatic()->GetTypeName(); } \
        static const PhxEngine::Core::TypeInfo* GetTypeInfoStatic() { static const PhxEngine::Core::TypeInfo typeInfoStatic(#typeName, BaseClassName::GetTypeInfoStatic()); return &typeInfoStatic; }


	class Object
	{
	public:
		Object();
		virtual ~Object();

		virtual StringHash GetType() const = 0;
		virtual const std::string& GetTypeName() const = 0;
        virtual const TypeInfo* GetTypeInfo() const = 0;

        /// Return type info static.
        static const TypeInfo* GetTypeInfoStatic() { return nullptr; }
        bool IsInstanceOf(StringHash type) const;
        bool IsInstanceOf(const TypeInfo* typeInfo) const;
        template<typename T> bool IsInstanceOf() const { return IsInstanceOf(T::GetTypeInfoStatic()); }
        template<typename T> T* Cast() { return IsInstanceOf<T>() ? static_cast<T*>(this) : nullptr; }
        template<typename T> const T* Cast() const { return IsInstanceOf<T>() ? static_cast<const T*>(this) : nullptr; }

	private:
		Core::Handle<ObjectId> m_trackerId;
	};

	namespace ObjectTracker
	{
		Core::Handle<ObjectId> RegisterInstance(Object* object);
		void RemoveInstance(Core::Handle<ObjectId> handle);

		void Finalize();
	}
}

