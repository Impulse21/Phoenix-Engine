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


#define PHXFIELD(StructType, member) \