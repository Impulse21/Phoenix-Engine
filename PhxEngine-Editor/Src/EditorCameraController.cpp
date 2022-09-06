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
	const float clampedDT = std::min(timestep.GetMilliseconds(), 0.1f); // if dt > 100 millisec, don't allow the camera to jump too far...
	const float speed = ((glfwGetKey(gltfWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 10.0f : 1.0f) * clampedDT;

	int state = glfwGetKey(gltfWindow, GLFW_KEY_W);
	if (state == GLFW_PRESS)
	{
		this->Walk(speed, camera);
	}

	state = glfwGetKey(gltfWindow, GLFW_KEY_S);
	if (state == GLFW_PRESS)
	{
		this->Walk(-speed, camera);
	}

	state = glfwGetKey(gltfWindow, GLFW_KEY_A);
	if (state == GLFW_PRESS)
	{
		this->Strafe(-speed, camera);
	}

	state = glfwGetKey(gltfWindow, GLFW_KEY_D);
	if (state == GLFW_PRESS)
	{
		this->Strafe(speed, camera);
	}

	static XMFLOAT2 slastPos = { 0.0f, 0.0f };
	double xpos, ypos;
	glfwGetCursorPos(gltfWindow, &xpos, &ypos);

	state = glfwGetMouseButton(gltfWindow, GLFW_MOUSE_BUTTON_MIDDLE);
	if (state == GLFW_PRESS)
	{

		float dx = XMConvertToRadians(0.25f * static_cast<float>(xpos - slastPos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(ypos - slastPos.y));

		this->Pitch(-dy, camera);
		this->RotateY(-dx, camera);
	}
	slastPos.x = xpos;
	slastPos.y = ypos;

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
