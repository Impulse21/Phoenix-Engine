#include "PhxData_pch.h"
#include "Reflection.h"
#include "DataTypeFactory.h"
#include "PhxCore/Base.h"
#include "PhxData/WorldChunk.def.h"


using namespace phx;
using namespace phx::data;

FieldInfo Component_Fields[] = {
	{ "_dummy", "uint8_t"_hash, "", phx_offsetof(&Component::_dummy), std::initializer_list<ExtraInfo>{{}}, false, false, false, 0, sizeof(uint8_t) },
};

TypeInfo Component_TypeInfo = {
	"Component", Component_Fields 
};

template<> const TypeInfo& Reflection<Component>::GetTypeInfo() { return Component_TypeInfo; }
const phx::data::TypeInfo& Component::GetTypeInfoStatic() { return Reflection<Component>::GetTypeInfo(); }
phx::StringHash Component::GetTypeIdStatic() { return Component::TypeId; }
const phx::data::TypeInfo& Component::GetBaseTypeInfoStatic() { return IDataObj::GetTypeInfoStatic(); }
phx::StringHash Component::GetBaseTypeIdStatic() { return IDataObj::GetTypeIdStatic(); }
REGISTER_TYPE_FACTORY(Component)

FieldInfo TransformComponent_Fields[] = {
	{ "Translation", "DirectX::XMFLOAT3"_hash, "", phx_offsetof(&TransformComponent::Translation), std::initializer_list<ExtraInfo>{{}}, false, false, false, 0, sizeof(DirectX::XMFLOAT3) },
	{ "Rotation", "DirectX::XMFLOAT4"_hash, "", phx_offsetof(&TransformComponent::Rotation), std::initializer_list<ExtraInfo>{{}}, false, false, false, 0, sizeof(DirectX::XMFLOAT4) },
	{ "Scale", "DirectX::XMFLOAT3"_hash, "", phx_offsetof(&TransformComponent::Scale), std::initializer_list<ExtraInfo>{{}}, false, false, false, 0, sizeof(DirectX::XMFLOAT3) },
};

TypeInfo TransformComponent_TypeInfo = {
	"TransformComponent", TransformComponent_Fields 
};

template<> const TypeInfo& Reflection<TransformComponent>::GetTypeInfo() { return TransformComponent_TypeInfo; }
const phx::data::TypeInfo& TransformComponent::GetTypeInfoStatic() { return Reflection<TransformComponent>::GetTypeInfo(); }
phx::StringHash TransformComponent::GetTypeIdStatic() { return TransformComponent::TypeId; }
const phx::data::TypeInfo& TransformComponent::GetBaseTypeInfoStatic() { return Component::GetTypeInfoStatic(); }
phx::StringHash TransformComponent::GetBaseTypeIdStatic() { return Component::GetTypeIdStatic(); }
REGISTER_TYPE_FACTORY(TransformComponent)

FieldInfo MeshComponent_Fields[] = {
	{ "Mesh", "std::string"_hash, "", phx_offsetof(&MeshComponent::Mesh), std::initializer_list<ExtraInfo>{{}}, false, false, false, 0, sizeof(std::string) },
};

TypeInfo MeshComponent_TypeInfo = {
	"MeshComponent", MeshComponent_Fields 
};

template<> const TypeInfo& Reflection<MeshComponent>::GetTypeInfo() { return MeshComponent_TypeInfo; }
const phx::data::TypeInfo& MeshComponent::GetTypeInfoStatic() { return Reflection<MeshComponent>::GetTypeInfo(); }
phx::StringHash MeshComponent::GetTypeIdStatic() { return MeshComponent::TypeId; }
const phx::data::TypeInfo& MeshComponent::GetBaseTypeInfoStatic() { return Component::GetTypeInfoStatic(); }
phx::StringHash MeshComponent::GetBaseTypeIdStatic() { return Component::GetTypeIdStatic(); }
REGISTER_TYPE_FACTORY(MeshComponent)

FieldInfo Entity_Fields[] = {
	{ "ID", "UUID"_hash, "", phx_offsetof(&Entity::ID), std::initializer_list<ExtraInfo>{{}}, false, false, false, 0, sizeof(UUID) },
	{ "Name", "std::string"_hash, "", phx_offsetof(&Entity::Name), std::initializer_list<ExtraInfo>{{}}, false, false, false, 0, sizeof(std::string) },
	{ "Components", "Component"_hash, "", phx_offsetof(&Entity::Components), std::initializer_list<ExtraInfo>{{}}, true, false, true, 0, sizeof(Component) },
	{ "Children", "Entity"_hash, "", phx_offsetof(&Entity::Children), std::initializer_list<ExtraInfo>{{}}, true, false, true, 0, sizeof(Entity) },
};

TypeInfo Entity_TypeInfo = {
	"Entity", Entity_Fields 
};

template<> const TypeInfo& Reflection<Entity>::GetTypeInfo() { return Entity_TypeInfo; }
const phx::data::TypeInfo& Entity::GetTypeInfoStatic() { return Reflection<Entity>::GetTypeInfo(); }
phx::StringHash Entity::GetTypeIdStatic() { return Entity::TypeId; }
const phx::data::TypeInfo& Entity::GetBaseTypeInfoStatic() { return IDataObj::GetTypeInfoStatic(); }
phx::StringHash Entity::GetBaseTypeIdStatic() { return IDataObj::GetTypeIdStatic(); }
REGISTER_TYPE_FACTORY(Entity)

FieldInfo WorldChunk_Fields[] = {
	{ "ID", "UUID"_hash, "", phx_offsetof(&WorldChunk::ID), std::initializer_list<ExtraInfo>{{}}, false, false, false, 0, sizeof(UUID) },
	{ "PackFile", "std::string"_hash, "", phx_offsetof(&WorldChunk::PackFile), std::initializer_list<ExtraInfo>{{}}, false, false, false, 0, sizeof(std::string) },
	{ "Root", "Entity"_hash, "", phx_offsetof(&WorldChunk::Root), std::initializer_list<ExtraInfo>{{}}, true, false, false, 0, sizeof(Entity) },
};

TypeInfo WorldChunk_TypeInfo = {
	"WorldChunk", WorldChunk_Fields 
};

template<> const TypeInfo& Reflection<WorldChunk>::GetTypeInfo() { return WorldChunk_TypeInfo; }
const phx::data::TypeInfo& WorldChunk::GetTypeInfoStatic() { return Reflection<WorldChunk>::GetTypeInfo(); }
phx::StringHash WorldChunk::GetTypeIdStatic() { return WorldChunk::TypeId; }
const phx::data::TypeInfo& WorldChunk::GetBaseTypeInfoStatic() { return IDataObj::GetTypeInfoStatic(); }
phx::StringHash WorldChunk::GetBaseTypeIdStatic() { return IDataObj::GetTypeIdStatic(); }
REGISTER_TYPE_FACTORY(WorldChunk)

const std::unordered_map<std::string, const TypeInfo*> g_TypeRegistry = {
	{ "Component", &Reflection<Component>::GetTypeInfo()},
	{ "TransformComponent", &Reflection<TransformComponent>::GetTypeInfo()},
	{ "MeshComponent", &Reflection<MeshComponent>::GetTypeInfo()},
	{ "Entity", &Reflection<Entity>::GetTypeInfo()},
	{ "WorldChunk", &Reflection<WorldChunk>::GetTypeInfo()},
};

