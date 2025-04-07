#include "PhxData_pch.h"
#include "Reflection.h"
#include "WorldComponents.def.h"
#include "PhxCore/Base.h"


using namespace phx;
using namespace phx::rft;

FieldInfo IDComponent_Fields[] = {
	{ "ID", "UUID"_hash, "", phx_offsetof(&IDComponent::ID), nullptr, std::initializer_list<ExtraInfo>{} },
};

TypeInfo IDComponent_TypeInfo = {
	"IDComponent", IDComponent_Fields 
};

template<> const TypeInfo& Refelction<IDComponent>::GetTypeInfo() { return IDComponent_TypeInfo; }
template<> constexpr StringHash Refelction<IDComponent>::GetTypeId() { return "IDComponent"_hash; }

FieldInfo NameComponent_Fields[] = {
	{ "Name", "std::string"_hash, "", phx_offsetof(&NameComponent::Name), nullptr, std::initializer_list<ExtraInfo>{} },
};

TypeInfo NameComponent_TypeInfo = {
	"NameComponent", NameComponent_Fields 
};

template<> const TypeInfo& Refelction<NameComponent>::GetTypeInfo() { return NameComponent_TypeInfo; }
template<> constexpr StringHash Refelction<NameComponent>::GetTypeId() { return "NameComponent"_hash; }

FieldInfo TransformComponent_Fields[] = {
	{ "Scale", "DirectX::XMFLOAT3"_hash, "Local Scale", phx_offsetof(&TransformComponent::LocalScale), nullptr, std::initializer_list<ExtraInfo>{} },
	{ "Rotation", "DirectX::XMFLOAT4"_hash, "Local Rotation", phx_offsetof(&TransformComponent::LocalRotation), nullptr, std::initializer_list<ExtraInfo>{} },
	{ "Translation", "DirectX::XMFLOAT3"_hash, "Local Translation", phx_offsetof(&TransformComponent::LocalTranslation), nullptr, std::initializer_list<ExtraInfo>{} },
};

TypeInfo TransformComponent_TypeInfo = {
	"TransformComponent", TransformComponent_Fields 
};

template<> const TypeInfo& Refelction<TransformComponent>::GetTypeInfo() { return TransformComponent_TypeInfo; }
template<> constexpr StringHash Refelction<TransformComponent>::GetTypeId() { return "TransformComponent"_hash; }

const std::unordered_map<std::string, const TypeInfo*> g_TypeRegistry = {
	{ "IDComponent", &Refelction<IDComponent>::GetTypeInfo()},
	{ "NameComponent", &Refelction<NameComponent>::GetTypeInfo()},
	{ "TransformComponent", &Refelction<TransformComponent>::GetTypeInfo()},
};

