#include "PhxWorld/PhxWorld_pch.h"

#include <PhxCore/VFS.h>

#include "WorldSerializer.h"
#include "World.h"
#include "WorldComponents.h"

#include <fstream>

#include <PhxWorld/Entity.h>
#include <PhxData/WorldComponents.def.h>
#include <PhxData/Reflection.h>

#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>

using namespace phx;

namespace
{
	void SerializeEntity(YAML::Emitter& out, phx::Entity entity, World& world)
	{
		PHX_CORE_ASSERT(entity.HasComponent<IDComponent>());

		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		if (entity.HasComponent<NameComponent>())
		{
			auto& component = entity.GetComponent<NameComponent>();
			rft::SerializeToYAML<NameComponent>(out, &component);
		}

		if (entity.HasComponent<HierarchyComponent>())
		{
			if (entity.HasComponent<HierarchyComponent>())
			{
				out << YAML::Key << "HierarchyComponent";
				out << YAML::BeginMap; // HierarchyComponent

				auto& hc = entity.GetComponent<HierarchyComponent>();
				Entity parentEntity(hc.ParentID, &world);

				PHX_CORE_ASSERT(parentEntity);
				out << YAML::Key << "Parent" << YAML::Value << parentEntity.GetUUID();
				out << YAML::EndMap; // HierarchyComponent
			}
		}

		if (entity.HasComponent<TransformComponent>())
		{
			auto& component = entity.GetComponent<TransformComponent>();
			rft::SerializeToYAML<TransformComponent>(out, &component);
		}

		out << YAML::EndMap; // Entity
	}
}

bool WorldSerializer::Save(IFileSystem* fs, const char* filename, World& world)
{
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "World" << YAML::Value << "Untitled";
	out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

	for (auto& entityId : world.GetRegistry().view<entt::entity>())
	{
		Entity entity(entityId, &world);
		if (!entity)
			continue;

		SerializeEntity(out, entity, world);
	};

	out << YAML::EndSeq;
	out << YAML::EndMap;
	const char* strData = out.c_str();
	fs->WriteFile(filename, Span(strData, strlen(strData)));

	return true;
}

void phx::WorldSerializer::Load(IFileSystem* /*fs*/, const char* /*filename*/, World& /*world*/)
{
}