#include "PhxWorld/PhxWorld_pch.h"

#include <PhxCore/VFS.h>

#include "WorldSerializer.h"
#include "World.h"
#include "WorldComponents.h"

#include <fstream>

#include <PhxWorld/Entity.h>

#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>

using namespace phx;


namespace YAML 
{
	template<>
	struct convert<DirectX::XMFLOAT3>
	{
		static Node encode(DirectX::XMFLOAT3 const& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(Node const& node, DirectX::XMFLOAT3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();

			return true;
		}
	};

	template<>
	struct convert<DirectX::XMFLOAT4>
	{
		static Node encode(DirectX::XMFLOAT4 const& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(Node const& node, DirectX::XMFLOAT4& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();

			return true;
		}
	};

	template<>
	struct convert<phx::UUID>
	{
		static Node encode(const phx::UUID& uuid)
		{
			Node node;
			node.push_back((uint64_t)uuid);
			return node;
		}

		static bool decode(const Node& node, phx::UUID& uuid)
		{
			uuid = node.as<uint64_t>();
			return true;
		}
	};
}

YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT3 const& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
	return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT4 const& v)
{
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
	return out;
}

namespace
{
	namespace Yaml
	{
		void WriteComponent(YAML::Emitter& out, TransformComponent const& component)
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			out << YAML::Key << "Translation" << YAML::Value << component.LocalTranslation;
			out << YAML::Key << "Rotation" << YAML::Value << component.LocalRotation;
			out << YAML::Key << "Scale" << YAML::Value << component.LocalScale;

			out << YAML::EndMap; // TransformComponent
		}

		void WriteComponent(YAML::Emitter& out, MeshRenderComponent const& )
		{
			out << YAML::Key << "MeshRenderComponent";
			out << YAML::BeginMap; // MeshRenderComponent

			// out << YAML::Key << "Mesh" << YAML::Value << component.MeshResource;

			out << YAML::EndMap; // MeshRenderComponent
		}

		void SerializeEntity(YAML::Emitter& out, phx::Entity entity, World& world)
		{
			PHX_CORE_ASSERT(entity.HasComponent<IDComponent>());

			out << YAML::BeginMap; // Entity
			out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

			if (entity.HasComponent<NameComponent>())
			{
				out << YAML::Key << "NameComponent";
				out << YAML::BeginMap; // NameComponent
				out << YAML::Key << "Name" << YAML::Value << entity.GetName().c_str();
				out << YAML::EndMap; // NameComponent
			}

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

			if (entity.HasComponent<TransformComponent>())
			{
				WriteComponent(out, entity.GetComponent<TransformComponent>());
			}

			if (entity.HasComponent<MeshRenderComponent>())
			{
				WriteComponent(out, entity.GetComponent<MeshRenderComponent>());
			}
			out << YAML::EndMap; // Entity
		}
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

		Yaml::SerializeEntity(out, entity, world);
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
