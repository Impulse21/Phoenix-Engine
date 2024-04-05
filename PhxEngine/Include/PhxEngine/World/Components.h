#pragma once

#include <PhxEngine/Core/UUID.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Core/RefCountPtr.h>

namespace PhxEngine
{
	class Mesh;

	// TODO: Move to a math library
	
	struct IDComponent
	{
		UUID ID;
	};

	struct NameComponent
	{
		std::string Name;

		inline void operator=(const std::string& str) { this->Name = str; }
		inline void operator=(std::string&& str) { this->Name = std::move(str); }
		inline bool operator==(const std::string& str) const { return this->Name.compare(str) == 0; }

	};

	struct TransformComponent
	{
		enum Flags
		{
			kEmpty = 0,
			kDirty = 1 << 0,
		};

		uint32_t Flags;

		DirectX::XMFLOAT3 LocalScale = { 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 LocalRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 LocalTranslation = { 0.0f, 0.0f, 0.0f };

		DirectX::XMFLOAT4X4 WorldMatrix = cIdentityMatrix;

	};

	struct HierarchyComponent
	{
		entt::entity ParentID = entt::null;
	};

	struct MeshComponent
	{
		RefCountPtr<Mesh> Mesh;
	};
}