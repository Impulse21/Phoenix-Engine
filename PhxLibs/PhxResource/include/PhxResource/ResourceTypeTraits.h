#pragma once

#include <PhxCore/StringHash.h>

namespace phx
{
    template<typename T>
    struct ResourceTraits
    {
        static constexpr const char* Extension = nullptr;
        static constexpr StringHash LoaderId = StringHash(0u);
    };
}
#define PHX_DEFINE_RESOURCE(TYPE, EXT, LOADER_NAME) \
    namespace phx { \
        template<> struct ResourceTraits<TYPE> { \
            static constexpr const char* Extension = EXT; \
            static constexpr StringHash LoaderId = LOADER_NAME##_hash; \
        }; \
    }