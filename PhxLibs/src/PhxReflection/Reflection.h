#pragma once

#include <vector>
#include <PhxReflection\TypeInfo.h>
#include "PhxReflection\ReflectionBuilder.h"

namespace phx::reflection
{
    void Initialize();
    void Shutdown();

    template<typename T>
    ReflectionBuilder<T> Reflect(const char* name)
    {
        return ReflectionBuilder<T>(name);
    }

    struct MemberInfo;

    class IReflectionRegistry
    {
    public:
        inline static IReflectionRegistry* Ptr = nullptr;
    public:

        virtual void RegisterType(const TypeInfo* typeInfo) = 0;
        virtual const TypeInfo* FindType(const char* name) const = 0;

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