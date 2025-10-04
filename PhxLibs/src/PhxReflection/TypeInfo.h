#pragma once

#include <PhxData/Any.h>
#include <PhxData/TemplatedTypeId.h>
#include <variant>
#include <vector>

namespace phx::reflection
{
	// Property metadata (for UI hints, serialization flags, etc.)
	using PropertyValue = std::variant<int, float, bool, const char*>;
	struct PropertyInfo { uint32_t m_key; PropertyValue m_value; };


    struct TypeInfo;
    struct MemberInfo
    {
        data::TemplateTypeId type;
        data::TemplateTypeId object_type;
        const char* name = nullptr;
        std::vector<PropertyInfo> properties;

        using GetValueFunction = data::Any(*)(data::AnyPtr object);
        using SetValueFunction = void (*)(data::AnyPtr object, data::Any value);
        using GetAddressFunction = data::AnyPtr(*)(data::AnyPtr object);

        GetValueFunction get_value;
        SetValueFunction set_value;
        GetAddressFunction get_address;


        // Operators
        bool operator==(const MemberInfo& other) const noexcept;
        std::strong_ordering operator<=>(const MemberInfo& other) const noexcept;
    };

    struct TypeInfo
    {
        const char* name = nullptr;
        size_t size = 0;
        const TypeInfo* parent = nullptr; // Pointer to the base class's TypeInfo
        std::vector<MemberInfo> members;

        // Helper to find a member by name (searches this class only)
        const MemberInfo* FindMember(const char* name) const
        {
            for (const auto& member : members)
            {
                if (strcmp(member.name, name) == 0)
                {
                    return &member;
                }
            }
            return nullptr;
        }
    };
}