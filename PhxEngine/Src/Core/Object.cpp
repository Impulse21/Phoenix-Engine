#include <PhxEngine/Core/Object.h>

using namespace PhxEngine;

PhxEngine::Object::Object() = default;
PhxEngine::Object::~Object() = default;

bool PhxEngine::Object::IsInstanceOf(StringHash type) const
{
	return this->GetTypeInfo()->IsTypeOf(type);
}

bool PhxEngine::Object::IsInstanceOf(const TypeInfo* typeInfo) const
{
	return this->GetTypeInfo()->IsTypeOf(typeInfo);
}


PhxEngine::TypeInfo::TypeInfo(const char* typeName, const TypeInfo* baseTypeInfo)
	: m_type(typeName)
	, m_typeName(typeName)
	, m_baseTypeInfo(baseTypeInfo)
{
}

bool PhxEngine::TypeInfo::IsTypeOf(StringHash type) const
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

bool PhxEngine::TypeInfo::IsTypeOf(const TypeInfo* typeInfo) const
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