#pragma once

#include <PhxCore/Span.h>
#include <PhxCore/StringHash.h>
#include <string>

namespace phx::data
{
    enum class FieldKind { Float, Int, Uint, Pointer, Float2, Float3, Float4 };

    struct FieldInfo
    {
        std::string_view Name;
        size_t Offset;
        size_t ElementSize;
        size_t ElementCount;
        FieldKind Kind;
        bool IsArray;
    };

    class TypeInfo
    {
        phx::StringHash Type;
        std::string_view TypeName;
        const TypeInfo* BaseTypeInfo;
        phx::Span<FieldInfo> Fields;
    };
}