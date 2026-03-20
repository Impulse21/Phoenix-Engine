#pragma once

#include <PhxCore/UUID.h>

template <>
struct rfl::Reflector<phx::UUID>
{
    using ReflType = std::string;

    static phx::UUID from(const std::string& str) noexcept
    {
        return phx::UUID::FromString(str);
    }

    static std::string to(const phx::UUID& id) noexcept
    {
        return id.ToString();
    }
};