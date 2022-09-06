#include "EditorCameraController.h"
#include "EditorCameraController.h"

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;

using namespace  DirectX;

#include <GLFW/glfw3.h>
#include <PhxEngine/App/Application.h>
#include <PhxEngine/Core/Window.h>

void EditorCameraController::OnUpdate(TimeStep const& timestep, New::CameraComponent& camera)
{
	auto* gltfWindow = static_cast<GLFWwindow*>(LayeredApplication::Ptr->GetWindow()->GetNativeWindow());
	assert(gltfWindow);

	int state = glfwGetKey(gltfWindow, GLFW_KEY_W);
	if (state == GLFW_PRESS)
	{
		this->Walk(0.1f * timestep.GetMilliseconds(), camera);
	}

	state = glfwGetKey(gltfWindow, GLFW_KEY_S);
	if (state == GLFW_PRESS)
	{
		this->Walk(-0.1f * timestep.GetMilliseconds(), camera);
	}

	state = glfwGetKey(gltfWindow, GLFW_KEY_A);
	if (state == GLFW_PRESS)
	{
		this->Strafe(-0.1f * timestep.GetMilliseconds(), camera);
	}

	state = glfwGetKey(gltfWindow, GLFW_KEY_D);
	if (state == GLFW_PRESS)
	{
		this->Walk(0.1f * timestep.GetMilliseconds(), camera);
	}

	camera.UpdateCamera();
}

void EditorCameraController::Walk(float d, PhxEngine::Scene::New::CameraComponent& camera)
{
	// position += d * lookV
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR l = XMLoadFloat3(&camera.Forward);
	XMVECTOR p = XMLoadFloat3(&camera.Eye);

	XMStoreFloat3(&camera.Eye, XMVectorMultiplyAdd(s, l, p));
}

void EditorCameraController::Strafe(float d, PhxEngine::Scene::New::CameraComponent& camera)
{
	// position += d * rightV
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR forward = XMLoadFloat3(&camera.Forward);
	XMVECTOR up = XMLoadFloat3(&camera.Up);
	XMVECTOR r = XMVector3Cross(forward, up);
	XMVECTOR p = XMLoadFloat3(&camera.Eye);

	XMStoreFloat3(&camera.Eye, XMVectorMultiplyAdd(s, r, p));
}

void EditorCameraController::Pitch(float angle, PhxEngine::Scene::New::CameraComponent& camera)
{
	// Rotate Up and look vector about the right vector
	XMVECTOR forward = XMLoadFloat3(&camera.Forward);
	XMVECTOR up = XMLoadFloat3(&camera.Up);
	XMVECTOR r = XMVector3Cross(forward, up);

	DirectX::XMMATRIX rotMtx = XMMatrixRotationAxis(r, angle);
	XMStoreFloat3(&camera.Up, XMVector3TransformNormal(up, rotMtx));
	XMStoreFloat3(&camera.Forward, XMVector3TransformNormal(forward, rotMtx));
}

void EditorCameraController::RotateY(float angle, PhxEngine::Scene::New::CameraComponent& camera)
{
	// Rotate the basis vectors anout the worl y-axis
	DirectX::XMMATRIX rotMtx = XMMatrixRotationY(angle);
	XMVECTOR forward = XMLoadFloat3(&camera.Forward);
	XMVECTOR up = XMLoadFloat3(&camera.Up);
	XMStoreFloat3(&camera.Up, XMVector3TransformNormal(up, rotMtx));
	XMStoreFloat3(&camera.Forward, XMVector3TransformNormal(forward, rotMtx));
}
