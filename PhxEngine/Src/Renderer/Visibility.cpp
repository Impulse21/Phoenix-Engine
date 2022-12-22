#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "Visibility.h"

#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Components.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;
using namespace PhxEngine::Renderer;

void PhxEngine::Renderer::FrustumCull(Scene::Scene* scene, const Scene::CameraComponent* cameraComp, uint32_t options, CullResults& outCullResults)
{
	// DO Culling
	outCullResults.VisibleMeshInstances.clear();
	outCullResults.Scene = scene;

	if ((options & CullOptions::FreezeCamera) != CullOptions::FreezeCamera)
	{
		outCullResults.Frustum = cameraComp->ViewProjectionFrustum;
		outCullResults.Eye = cameraComp->Eye;
	}

	auto view = outCullResults.Scene->GetAllEntitiesWith<MeshInstanceComponent, AABBComponent>();
	for (auto e : view)
	{
		auto [meshInstanceComponent, aabbComponent] = view.get<MeshInstanceComponent, AABBComponent>(e);

		if (outCullResults.Frustum.CheckBoxFast(aabbComponent))
		{
			outCullResults.VisibleMeshInstances.push_back({ e, outCullResults.Scene });
		}
	}

}
