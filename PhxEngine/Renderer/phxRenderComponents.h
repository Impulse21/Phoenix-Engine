#pragma once

#include "RHI/PhxRHI.h"
#include "entt/entt.hpp"
#include "Core/phxPrimitives.h"
namespace phx
{
	struct MeshComponent
	{
		entt::entity Material;
		AABB Aabb;
		Sphere BoundingSphere;

		rhi::BufferHandle VertexBuffer;
		rhi::BufferHandle IndexBuffer;
	};
}