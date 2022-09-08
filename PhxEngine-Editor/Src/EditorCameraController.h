#pragma once

// This is to clear #error:  "No Target Architecture" due to DirectXMath. Not sure how to get around this.
#include <Windows.h>
#include "PhxEngine/Scene/Components.h"
#include "PhxEngine/Core/TimeStep.h"

class EditorCameraController
{
public:
	EditorCameraController() = default;
	~EditorCameraController() = default;

	void OnUpdate(
		PhxEngine::Core::TimeStep const& timestep,
		PhxEngine::Scene::CameraComponent& camera);

private:
	void Walk(float d, PhxEngine::Scene::CameraComponent& camera);
	void Strafe(float d, PhxEngine::Scene::CameraComponent& camera);
	void Pitch(float angle, PhxEngine::Scene::CameraComponent& camera);
	void RotateY(float angle, PhxEngine::Scene::CameraComponent& camera);


};

