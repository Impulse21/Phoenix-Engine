#include "PhxData_pch.h"
#include "Reflection.h"
#include "WorldComponents.def.h"


using namespace phx;
using namespace phx::rft;

constexpr FieldInfo IDComponent_Fields[] = {
	{ "ID", "UUID"_hash, "", offsetof(IDComponent, ID), nullptr, std::initializer_list<ExtraInfo>{} },
};

constexpr TypeInfo IDComponent_TypeInfo = {
	"IDComponent", IDComponent_Fields 
};

template<> const TypeInfo& Refelction<IDComponent>::GetTypeInfo() { return IDComponent_TypeInfo; }
template<> constexpr StringHash Refelction<IDComponent>::GetTypeId() { return "IDComponent"_hash; }

constexpr FieldInfo TransformComponent_Fields[] = {
	{ "Scale", "DirectX::XMFLOAT3"_hash, "Local Scale", offsetof(TransformComponent, LocalScale), nullptr, std::initializer_list<ExtraInfo>{} },
	{ "Rotation", "DirectX::XMFLOAT4"_hash, "Local Rotation", offsetof(TransformComponent, LocalRotation), nullptr, std::initializer_list<ExtraInfo>{} },
	{ "Translation", "DirectX::XMFLOAT3"_hash, "Local Translation", offsetof(TransformComponent, LocalTranslation), nullptr, std::initializer_list<ExtraInfo>{} },
};

constexpr TypeInfo TransformComponent_TypeInfo = {
	"TransformComponent", TransformComponent_Fields 
};

template<> const TypeInfo& Refelction<TransformComponent>::GetTypeInfo() { return TransformComponent_TypeInfo; }
template<> constexpr StringHash Refelction<TransformComponent>::GetTypeId() { return "TransformComponent"_hash; }

const std::unordered_map<std::string, const TypeInfo*> g_TypeRegistry = {
	{ "IDComponent", &Refelction<IDComponent>::GetTypeInfo()},
	{ "TransformComponent", &Refelction<TransformComponent>::GetTypeInfo()},
};

