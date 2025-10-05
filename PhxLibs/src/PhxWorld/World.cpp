#include "PhxWorld_pch.h"

#include "Entity.h"

#include "WorldComponents.h"
#include "WorldMetadata.def.h"

#include <PhxResource/ResourceSystem.h>
#include <DirectXMath.h>

#include <algorithm>

using namespace phx;
using namespace DirectX;

void phx::World::RegisterReflection()
{
	TransformComponent::Reflect();
	MeshComponent::Reflect();
}

void phx::World::InstantiateFrom(SceneBlueprint& scene_reader)
{
	// Add entities
	for (size_t root_node_handle : scene_reader.root_node_indices)
	{
		SceneNode* node = scene_reader.GetNode(root_node_handle);

		if (!node)
			continue;

		
		Entity entity = CreateEntity(node->name); 
		node->runtime_entity = entity;

		for (const auto& compoenent : node->components)
		{
			switch (compoenent->type_hash.ToHash())
			{
			case scene::MeshComponent::StaticTypeHash():
				auto* scene_mesh_comp = static_cast<scene::MeshComponent*>(compoenent.get());
				auto& mesh_component = entity.AddComponent<MeshComponent>();
				mesh_component.Mesh = phx::ResourceSystem::Ptr->Get(scene_mesh_comp->mesh_virtual_path.c_str());
				
				// TODO: Material Assignmnet
				break;
			}
		}
	}
}

Entity World::CreateEntity(std::string const& name)
{
	return CreateEntity(UUID(), name);
}

Entity World::CreateEntity(UUID uuid, std::string const& name)
{
	Entity entity = { m_registry.create(), this };
	entity.AddComponent<IDComponent>(uuid);
	entity.AddComponent<TransformComponent>();
	auto& nameComp = entity.AddComponent<NameComponent>();
	nameComp.Name = name.empty() ? "Entity" : name;
	return entity;
}

void World::DestroyEntity(Entity entity)
{
	DetachChildren(entity);
	m_registry.destroy(entity);
}

void World::AttachToParent(Entity entity, Entity parent, bool childInLocalSpace)
{
	assert(entity != parent);
	entity.AttachToParent(parent, childInLocalSpace);
}

void World::DetachFromParent(Entity entity)
{
	entity.DetachFromParent();
}

void World::DetachChildren(Entity parent)
{
	parent.DetachChildren();
}

void phx::World::RegisterComponents()
{
#if false
	entt::meta<TransformComponent>()
		.data<&TransformComponent::LocalTranslation>("LocalTranslation"_hs);
#endif
}
