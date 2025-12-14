#pragma once

#include <PhxCore/StringHash.h>

#include <variant> // for monostate

namespace phx
{
    template<typename T>
    struct ResourceTraits
    {
        using Hot = T;
        using Cold = std::monostate;
        
        static constexpr const char* Extension = nullptr;
        static constexpr StringHash LoaderId = StringHash(0u);
    };
}
#define PHX_DEFINE_RESOURCE(TYPE, HOT, COLD, EXT, LOADER_NAME) \
    namespace phx { \
        template<> struct ResourceTraits<TYPE> { \
            using Hot = HOT; \
            using Cold = COLD; \
            static constexpr const char* Extension = EXT; \
            static constexpr StringHash LoaderId = LOADER_NAME##_hash; \
        }; \
    }