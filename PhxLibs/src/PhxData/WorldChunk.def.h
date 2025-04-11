#pragma once

#include <string>

#include <PhxCore/Base.h>
#include <PhxCore/UUID.h>

#include <DirectXMath.h>

#include "DataObject.h"

#include <PhxCore/RefCountPtr.h>
namespace phx::data
{
	struct Component : public DataObj
	{
		PHX_DATA_OBJECT(Component, DataObj)
	};

	struct TransformComponent : public Component
	{
		PHX_DATA_OBJECT(TransformComponent, Component)

		PROPERTY()
		DirectX::XMFLOAT3 Translation = { 0.0f, 0.0f, 0.0f };

		PROPERTY()
		DirectX::XMFLOAT4 Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

		PROPERTY()
		DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
	};

	struct MeshComponent : public Component
	{
		PHX_DATA_OBJECT(MeshComponent, Component)

		PROPERTY()
		std::string Mesh;
	};

	struct Entity : public DataObj
	{
		PHX_DATA_OBJECT(Entity, DataObj)

		PROPERTY()
		UUID ID;

		PROPERTY()
		std::string Name;

		PROPERTY()
		std::vector<RefCountPtr<Component>> Components;

		PROPERTY()
		std::vector<RefCountPtr<Entity>> Children;
	};

	struct WorldChunk : public DataObj
	{
		PHX_DATA_OBJECT(WorldChunk, DataObj)

		PROPERTY()
		UUID ID;

		PROPERTY()
		std::string PackFile;

		PROPERTY()
		std::vector<RefCountPtr<Entity>> Children;
	};
}