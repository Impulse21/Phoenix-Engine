#include "PhxData_pch.h"
#include "Reflection.h"
#include "WorldComponents.def"


using namespace phx;
using namespace phx::rft;

static const FieldInfo TransformComponent_Fields[] = {
	{ "Scale", "DirectX::XMFLOAT3"_hash, "Local Scale", offsetof(TransformComponent, LocalScale), nullptr, std::initializer_list<ExtraInfo>{} },
	{ "Rotation", "DirectX::XMFLOAT4"_hash, "Local Rotation", offsetof(TransformComponent, LocalRotation), nullptr, std::initializer_list<ExtraInfo>{} },
	{ "Translation", "DirectX::XMFLOAT3"_hash, "Local Translation", offsetof(TransformComponent, LocalTranslation), nullptr, std::initializer_list<ExtraInfo>{} },
};

static const TypeInfo TransformComponent_TypeInfo[] = {
	{ "TransformComponent", TransformComponent_Fields }
};

const std::unordered_map<std::string, const TypeInfo*> g_TypeRegistry = {
	{ "TransformComponent", TransformComponent_TypeInfo}
};

