#include "PhxWorld/PhxWorld_pch.h"
#include "WorldSerializer.h"
#include "World.h"

#include <fstream>

#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>

namespace
{
	struct YamlSerializer
	{
		template<typename Type>
		void operator()(entt::entity& entity, Type& component)
		{
		}
		// Output.
		void operator()(unsigned int entity)
		{
			// Not really sure why this is getting called. Need it defined to prevent compile errors.
		}


		void operator()(entt::entity entity)
		{
		}


		template<typename Type>
		void operator()(entt::entity entity, const Type& component)
		{
		}

		YamlSerializer(YAML::Emitter& emitter)
			: Emitter(emitter)
		{
		};

		YAML::Emitter& Emitter;
	};
}
bool phx::WorldSerializer::Save(const char* filename, World& world)
{
	YAML::Emitter emitter;
	emitter << YAML::BeginSeq;

	YamlSerializer serializer(emitter);

	entt::snapshot snapshot{ world.GetRegistry() }.get<entt::entity>();

	// Serialize entities manually (since snapshot no longer tracks them)
	world.GetRegistry().view.each([&](auto entity)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Entity" << YAML::Value << static_cast<uint32_t>(entity);
			out << YAML::EndMap;
		});

	snapshot->([&](auto entity)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Entity" << YAML::Value << static_cast<uint32_t>(entity);
			out << YAML::EndMap;
		});

	std::ofstream outStream("Testing.yaml");
	outStream << node;

	outStream.close();
	return true;
}

void phx::WorldSerializer::Load(const char* filename, World& world)
{
}
