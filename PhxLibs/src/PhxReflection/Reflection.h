#pragma once

#include <vector>
#include <PhxReflection/TypeInfo.h>
#include "PhxReflection/ReflectionBuilder.h"

// 1. The macro to be placed inside each reflectable struct/class
#define PHX_REFLECTION_VARS() \
    static phx::reflection::TypeInfo s_type_info;
#define PHX_DEFINE_REFLECTION() \
    static inline void RegisterReflectionBody()

#define PHX_REGISTER_REFLECTION(type) type::RegisterReflectionBody()

namespace phx::reflection
{
    void Initialize();
    void Shutdown();

    template<typename T>
    ReflectionBuilder<T> Reflect(const char* name)
    {
        return ReflectionBuilder<T>(name);
    }

    class IReflectionRegistry
    {
    public:
        inline static IReflectionRegistry* Ptr = nullptr;
    public:

        virtual void RegisterType(const TypeInfo* typeInfo) = 0;
        virtual const TypeInfo* FindType(const char* name) const = 0;

        virtual ~IReflectionRegistry() = default;
    };

    // Template helper for convenience
    template<typename T>
    inline const TypeInfo* FindType()
    {
#if false
        // We'll define a helper GetTypeInfo<T>() later
        extern const TypeInfo* GetTypeInfo_Internal<T>();
        return GetTypeInfo_Internal<T>();
#else
        return nullptr;
#endif
    }
}