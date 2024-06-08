#pragma once

#include "Core/phxLog.h"

#define PHX_UNUSED [[maybe_unused]]


struct NonMoveable
{
    NonMoveable() = default;
    NonMoveable(NonMoveable&&) = delete;
    NonMoveable& operator = (NonMoveable&&) = delete;
};

struct NonCopyable
{
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};