#pragma once

#include <vector>

// TODO: Use Any
#include <variant>

namespace phx::reflection
{
	// Property metadata (for UI hints, serialization flags, etc.)
	using PropertyValue = std::variant<int, float, bool, const char*>;
	struct PropertyInfo { uint32_t m_key; PropertyValue m_value; };

    struct TypeInfo;
    struct MemberInfo
    {
        const char* name = nullptr;
        const TypeInfo* type = nullptr;
        size_t offset = 0;
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