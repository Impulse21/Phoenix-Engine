#pragma once

#include <concepts>

namespace phx::ecs
{
    template<typename T>
    concept HasRequired = requires { typename T::Required; };

    template<typename T>
    struct ComponentPolicy
    {
        static constexpr bool IsSingleton = false;
    };

    template<typename T>
    constexpr bool is_singleton_v = ComponentPolicy<T>::IsSingleton;
}

#define PHX_SINGLETON_COMPONENT(T) \
    template<> struct phx::ecs::ComponentPolicy<T> { static constexpr bool IsSingleton = true; }