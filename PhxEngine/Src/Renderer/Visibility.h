#pragma once


#include "PhxEngine/Core/Primitives.h"
#include <entt.hpp>
#include <PhxEngine/Core/Primitives.h>
#include <PhxEngine/Scene/Entity.h>

namespace PhxEngine::Scene
{
	class Scene;
	struct CameraComponent;
}

namespace PhxEngine::Renderer
{
	struct CullResults
	{
		Scene::Scene* Scene;
		Core::Frustum Frustum;
		DirectX::XMFLOAT3 Eye;

		std::vector<Scene::Entity> VisibleMeshInstances;
	};

	enum CullOptions
	{
		None = 0,
		FreezeCamera,
	};

	void FrustumCull(Scene::Scene* scene, const Scene::CameraComponent* cameraComp, uint32_t options, CullResults& outCullResults);
}