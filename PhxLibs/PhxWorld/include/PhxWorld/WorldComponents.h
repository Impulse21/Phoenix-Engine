#pragma once

#include <PhxCore/Base.h>
#include <PhxCore/UUID.h>
#include <PhxCore/Math.h>
#include <PhxCore/StaticArray.h>

#include <PhxRenderer/MeshResource.h>

#include <entt/entt.hpp>

#include <string>

#include <hlsl++.h>


namespace phx
{
	struct IDComponent
	{
		UUID ID;
	};

	struct NameComponent
	{
		std::string Name;

		inline void operator=(const std::string& str) { Name = str; }
		inline void operator=(std::string&& str) { Name = std::move(str); }
		inline bool operator==(const std::string& str) const { return Name.compare(str) == 0; }
	};

	struct HierarchyComponent
	{
		entt::entity ParentID = entt::null;
	};

	struct CameraComponent
	{
		float width = 0.0f;
		float height = 0.0f;
		float z_near = 0.1f;
		float z_far = 5000.0f;
		float fov = 1.0f; // Radians

		hlslpp::float3 eye = { 0.0f, 0.0f, 0.0f };
#ifdef LH
		DirectX::XMFLOAT3 Forward = { 0.0f, 0.0f, 1.0f };
#else
		hlslpp::float3 forward = { 0.0f, 0.0f, -1.0f };
#endif
		hlslpp::float3 up = { 0.0f, 1.0f, 0.0f };

		bool active : 1 = false;
	};

	struct alignas(64) StaticMeshComponent
	{
		RefCountPtr<phx::renderer::MeshResource> mesh;
		uint32_t* material_ids;
		uint8_t num_materials;
		uint8_t layer_mask;
		bool visible;
		uint8_t _padding[5];
	};
	static_assert(sizeof(StaticMeshComponent) <= 64);

	struct alignas(64) StaticMeshStorageComponent
	{
		RefCountPtr<phx::renderer::MeshResource> mesh;
		StaticArray<uint32_t, 8> materials_ids;
	};
	static_assert(sizeof(StaticMeshComponent) <= 64);

	struct alignas(16) TransformComponent
	{
		union
		{
			struct
			{
				uint8_t dirty : 1;
				uint8_t _unused : 7;
			};
			uint8_t Flags;
		};

		hlslpp::float3 scale		= { 1.0f, 1.0f, 1.0f };
		hlslpp::quaternion rotation	= { 0.0f, 0.0f, 0.0f, 1.0f };
		hlslpp::float3 translation	= { 0.0f, 0.0f, 0.0f };

		inline bool IsDirty() const { return dirty; }
	};
	static_assert(sizeof(TransformComponent) <= 64);

	struct alignas(16) WorldTransformComponent
	{
		hlslpp::float4x4 world_matrix = hlslpp::float4x4::identity();
	};
	static_assert(sizeof(WorldTransformComponent) <= 64);

}