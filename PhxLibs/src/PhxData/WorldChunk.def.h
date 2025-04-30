#pragma once

#include <string>

#include <PhxCore/Base.h>
#include <PhxCore/UUID.h>
#include <PhxCore/StringHash.h>

#include <PhxData/DataPtr.h>

#include <DirectXMath.h>

#include <PhxCore/RefCountPtr.h>

#define WORLD_COMPONENT(type) type() : Component(#type##_hash) {};

namespace YAML
{
	class Emitter; 
	class Node;
}

namespace phx
{
	class IFileSystem;
}

namespace phx::data
{
	struct Component
	{
		StringHash ComponentId;
		std::string ComponentName;
		Component(StringHash id)
			: ComponentId(id)
		{
		}

		virtual ~Component() = default;
		virtual void Serialize(YAML::Emitter& emitter) const = 0;
		virtual void  Deserialize(YAML::Node& node) = 0;
	};

	// NOTE: Polymorphism increases the size of these structs form
	// 40 bytes to 94 bytes 
	struct TransformComponent : public Component
	{
		WORLD_COMPONENT(TransformComponent)
		DirectX::XMFLOAT3 Translation = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT4 Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };

		void Serialize(YAML::Emitter& emitter) const override;
		void Deserialize(YAML::Node& node) override;
	};

	struct MeshComponent : public Component
	{
		WORLD_COMPONENT(MeshComponent)
		std::string Mesh;

		void Serialize(YAML::Emitter& emitter) const override;
		void Deserialize(YAML::Node& node) override;
	};

	struct Entity
	{
		UUID ID;
		std::string Name;
		std::vector<RefPtr<Component>> Components;
		std::vector<RefPtr<Entity>> Children;

		void Serialize(YAML::Emitter& emitter) const;
		void Deserialize(YAML::Node& node);
	};

	struct WorldChunk
	{
		UUID ID;
		std::string PackFile;
		RefPtr<Entity> Root;

		void Serialize(YAML::Emitter& emitter) const;
		void Deserialize(YAML::Node& node);
	};

	void Save(phx::IFileSystem* fs, const char* filename, WorldChunk const& chunk);
	data::RefPtr<WorldChunk> Load(phx::IFileSystem* fs, const char* filename);
}