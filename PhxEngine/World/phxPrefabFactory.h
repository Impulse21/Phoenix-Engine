#pragma once

#include "entt/entt.hpp"
#include "World/phxEntity.h"
#include "RHI/PhxRHI.h"
#include "Core/phxRefCountPtr.h"

namespace phx
{
	namespace rhi
	{
		class GfxDevice;
	}

	namespace PrefabFactory
	{
		Entity CreateCube(
			rhi::GfxDevice* gfxDevice,
			entt::entity matId = entt::null,
			float size = 1,
			bool rhsCoord = false);

		Entity CreateSphere(
			rhi::GfxDevice* gfxDevice,
			entt::entity matId = entt::null,
			float diameter = 1,
			size_t tessellation = 16,
			bool rhcoords = false);

		Entity CreatePlane(
			rhi::GfxDevice* gfxDevice,
			entt::entity matId = entt::null,
			float width = 1,
			float height = 1,
			bool rhCoords = false);
	}
}

