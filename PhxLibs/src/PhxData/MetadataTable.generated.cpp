#include "PhxData_pch.h"
#include "Reflection.h"
#include "WorldComponents.def.h"


using namespace phx;
using namespace phx::rft;

constexpr FieldInfo TransformComponent__Fields[] = {
	{ "Scale", "DirectX::XMFLOAT3"_hash, "Local Scale", offsetof(TransformComponent_, LocalScale), nullptr, std::initializer_list<ExtraInfo>{} },
	{ "Rotation", "DirectX::XMFLOAT4"_hash, "Local Rotation", offsetof(TransformComponent_, LocalRotation), nullptr, std::initializer_list<ExtraInfo>{} },
	{ "Translation", "DirectX::XMFLOAT3"_hash, "Local Translation", offsetof(TransformComponent_, LocalTranslation), nullptr, std::initializer_list<ExtraInfo>{} },
};

constexpr TypeInfo TransformComponent__TypeInfo = {
	"TransformComponent_", TransformComponent__Fields 
};

template<> const TypeInfo& Refelction<TransformComponent_>::GetTypeInfo() { return TransformComponent__TypeInfo; }
template<> constexpr StringHash Refelction<TransformComponent_>::GetTypeId() { return "TransformComponent_"_hash; }

const std::unordered_map<std::string, const TypeInfo*> g_TypeRegistry = {
	{ "TransformComponent_", &Refelction<TransformComponent_>::GetTypeInfo()}
};

