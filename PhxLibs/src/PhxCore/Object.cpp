#include "PhxCore/PhxCore_pch.h"

#include "Object.h"

phx::TypeInfo::TypeInfo(const char* typeName, const TypeInfo* baseTypeInfo, phx::Span<FieldInfo> fields)
	: m_type(typeName)
	, m_typeName(typeName)
	, m_baseTypeInfo(baseTypeInfo)
    , m_fieldInfo(fields)
{
}

bool phx::TypeInfo::IsTypeOf(StringHash type) const
{
    const TypeInfo* current = this;
    while (current)
    {
        if (current->GetType() == type)
            return true;

        current = current->GetBaseTypeInfo();
    }

    return false;
}

bool phx::TypeInfo::IsTypeOf(const TypeInfo* typeInfo) const
{
    if (typeInfo == nullptr)
        return false;

    const TypeInfo* current = this;
    while (current)
    {
        if (current == typeInfo || current->GetType() == typeInfo->GetType())
            return true;

        current = current->GetBaseTypeInfo();
    }

    return false;
}


