#include "PhxData_pch.h"
#include "Reflection.h"
#include "DataTypeFactory.h"
#include "PhxCore/Base.h"
#include "PhxData/WorldChunk.def.h"


using namespace phx;
using namespace phx::data;

FieldInfo TransformComponent_Fields[] = {
	{ "Translation", "DirectX::XMFLOAT3"_hash, "", phx_offsetof(&TransformComponent::Translation), std::initializer_list<ExtraInfo>{{}}, false },
	{ "Rotation", "DirectX::XMFLOAT4"_hash, "", phx_offsetof(&TransformComponent::Rotation), std::initializer_list<ExtraInfo>{{}}, false },
	{ "Scale", "DirectX::XMFLOAT3"_hash, "", phx_offsetof(&TransformComponent::Scale), std::initializer_list<ExtraInfo>{{}}, false },
};

TypeInfo TransformComponent_TypeInfo = {
	"TransformComponent", TransformComponent_Fields 
};

template<> const TypeInfo& Reflection<TransformComponent>::GetTypeInfo() { return TransformComponent_TypeInfo; }
REGISTER_TYPE_FACTORY(TransformComponent)

FieldInfo MeshComponent_Fields[] = {
	{ "Mesh", "std::string"_hash, "", phx_offsetof(&MeshComponent::Mesh), std::initializer_list<ExtraInfo>{{}}, false },
};

TypeInfo MeshComponent_TypeInfo = {
	"MeshComponent", MeshComponent_Fields 
};

template<> const TypeInfo& Reflection<MeshComponent>::GetTypeInfo() { return MeshComponent_TypeInfo; }
REGISTER_TYPE_FACTORY(MeshComponent)

FieldInfo Entity_Fields[] = {
	{ "ID", "UUID"_hash, "", phx_offsetof(&Entity::ID), std::initializer_list<ExtraInfo>{{}}, false },
	{ "Name", "std::string"_hash, "", phx_offsetof(&Entity::Name), std::initializer_list<ExtraInfo>{{}}, false },
	{ "Components", "std::vector<RefCountPtr<Component>>"_hash, "", phx_offsetof(&Entity::Components), std::initializer_list<ExtraInfo>{{}}, false },
	{ "Children", "std::vector<RefCountPtr<Entity>>"_hash, "", phx_offsetof(&Entity::Children), std::initializer_list<ExtraInfo>{{}}, false },
};

TypeInfo Entity_TypeInfo = {
	"Entity", Entity_Fields 
};

template<> const TypeInfo& Reflection<Entity>::GetTypeInfo() { return Entity_TypeInfo; }
REGISTER_TYPE_FACTORY(Entity)

FieldInfo WorldChunk_Fields[] = {
	{ "ID", "UUID"_hash, "", phx_offsetof(&WorldChunk::ID), std::initializer_list<ExtraInfo>{{}}, false },
	{ "PackFile", "std::string"_hash, "", phx_offsetof(&WorldChunk::PackFile), std::initializer_list<ExtraInfo>{{}}, false },
	{ "Children", "std::vector<RefCountPtr<Entity>>"_hash, "", phx_offsetof(&WorldChunk::Children), std::initializer_list<ExtraInfo>{{}}, false },
};

TypeInfo WorldChunk_TypeInfo = {
	"WorldChunk", WorldChunk_Fields 
};

template<> const TypeInfo& Reflection<WorldChunk>::GetTypeInfo() { return WorldChunk_TypeInfo; }
REGISTER_TYPE_FACTORY(WorldChunk)

const std::unordered_map<std::string, const TypeInfo*> g_TypeRegistry = {
	{ "TransformComponent", &Reflection<TransformComponent>::GetTypeInfo()},
	{ "MeshComponent", &Reflection<MeshComponent>::GetTypeInfo()},
	{ "Entity", &Reflection<Entity>::GetTypeInfo()},
	{ "WorldChunk", &Reflection<WorldChunk>::GetTypeInfo()},
};

