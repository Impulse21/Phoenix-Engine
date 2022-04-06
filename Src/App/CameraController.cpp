#include <PhxEngine/App/CameraController.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Renderer/SceneNodes.h>
#include <PhxEngine/Renderer/SceneComponents.h>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <DirectXMath.h>

using namespace PhxEngine;
using namespace PhxEngine::Renderer;
using namespace DirectX;


class DebugCameraController : public ICameraController
{
public:
	DebugCameraController(CameraNode& camera, XMVECTOR const& worldUp)
		: m_camera(camera)
	{
		bool joystickFound = glfwJoystickPresent(this->m_joystickId);
		assert(joystickFound);
		if (!joystickFound)
		{
			LOG_WARN("Unable to locate joystick. Please use keyboard");
		}

		this->m_worldUp = XMVector3Normalize(worldUp);
		this->m_worldNorth = XMVector3Normalize(XMVector3Cross(g_XMIdentityR0, this->m_worldUp));
		this->m_worldEast = XMVector3Cross(this->m_worldUp, this->m_worldNorth);

		// Construct current pitch
		// this->m_pitch = std::sin(XMVectorGetX(XMVector3Dot(this->m_worldUp, this->m_camera.GetForwardVector())));

		//XMVECTOR forward = XMVector3Normalize(XMVector3Cross(this->m_camera.GetRightVector(), this->m_worldUp));
		//this->m_yaw = atan2(-XMVectorGetX(XMVector3Dot(this->m_worldEast, forward)), XMVectorGetX(XMVector3Dot(this->m_worldNorth, forward)));

	}

	void Update(double elapsedTime)
	{
		GLFWgamepadstate state;

		if (!glfwGetGamepadState(this->m_joystickId, &state))
		{
			return;
		}


		this->m_pitch += this->GetAxisInput(state, GLFW_GAMEPAD_AXIS_RIGHT_Y) * this->m_lookSpeed;
		this->m_yaw += this->GetAxisInput(state, GLFW_GAMEPAD_AXIS_RIGHT_X) * this->m_lookSpeed;

		// Max out pitich
		this->m_pitch = XMMin(XM_PIDIV2, this->m_pitch);
		this->m_pitch = XMMax(-XM_PIDIV2, this->m_pitch);

		if (this->m_yaw > XM_PI)
		{
			this->m_yaw -= XM_2PI;
		}
		else if (this->m_yaw <= -XM_PI)
		{
			this->m_yaw += XM_2PI;
		}

		const XMMATRIX worldBase(this->m_worldEast, this->m_worldUp, this->m_worldNorth, g_XMIdentityR3);
		XMMATRIX orientation = worldBase * XMMatrixRotationX(this->m_pitch) * XMMatrixRotationY(this->m_yaw);
		XMVECTOR rotation = DirectX::XMQuaternionRotationMatrix(orientation);

		float forward = this->GetAxisInput(state, GLFW_GAMEPAD_AXIS_LEFT_Y) * this->m_movementSpeed;
		float strafe = this->GetAxisInput(state, GLFW_GAMEPAD_AXIS_LEFT_X) * this->m_strafeMovementSpeed;

		XMVECTOR translation = XMVectorSet(strafe, 0.0f, -forward, 1.0f);
		XMVECTOR currentTranslation = DirectX::XMLoadFloat3(&this->m_camera.GetTranslation());
		translation = XMVector3TransformNormal(translation, orientation) + currentTranslation;

		this->m_camera.SetTranslation(translation);
		this->m_camera.SetRotation(rotation);

		this->m_camera.UpdateViewMatrixLH();

	}

	void EnableDebugWindow(bool enabled) { this->m_enableDebugWindow = enabled; }

private:
	float GetAxisInput(GLFWgamepadstate const& state, int inputId)
	{
		auto value = state.axes[inputId];
		return value > this->DeadZone || value < -this->DeadZone
			? value
			: 0.0f;
	}

private:
	const int m_joystickId = GLFW_JOYSTICK_1;
	const float DeadZone = 0.2f;

	float m_movementSpeed = 0.3f;
	float m_strafeMovementSpeed = 0.08f;
	float m_lookSpeed = 0.01f;

	float m_pitch = 0.0f;
	float m_yaw = 0.0f;
	CameraNode& m_camera;

	bool m_enableDebugWindow = false;

	XMVECTOR m_worldUp;
	XMVECTOR m_worldNorth;
	XMVECTOR m_worldEast;
};


class DebugCameraController2 : public ICameraController
{
public:
	DebugCameraController2(CameraComponent& camera, XMVECTOR const& worldUp)
		: m_camera(camera)
	{
		bool joystickFound = glfwJoystickPresent(this->m_joystickId);
		assert(joystickFound);
		if (!joystickFound)
		{
			LOG_WARN("Unable to locate joystick. Please use keyboard");
		}

		this->m_worldUp = XMVector3Normalize(worldUp);
		this->m_worldNorth = XMVector3Normalize(XMVector3Cross(g_XMIdentityR0, this->m_worldUp));
		this->m_worldEast = XMVector3Cross(this->m_worldUp, this->m_worldNorth);

		// Construct current pitch
		// this->m_pitch = std::sin(XMVectorGetX(XMVector3Dot(this->m_worldUp, this->m_camera.GetForwardVector())));

		//XMVECTOR forward = XMVector3Normalize(XMVector3Cross(this->m_camera.GetRightVector(), this->m_worldUp));
		//this->m_yaw = atan2(-XMVectorGetX(XMVector3Dot(this->m_worldEast, forward)), XMVectorGetX(XMVector3Dot(this->m_worldNorth, forward)));

	}

	void Update(float elapsedTime)
	{
		GLFWgamepadstate state;

		if (!glfwGetGamepadState(this->m_joystickId, &state))
		{
			return;
		}

		float xDiff = 0.0f;
		float yDiff = 0.0f;

		// -- Update keyboard state ---
		const float buttonRotationSpeed = 2.0 * elapsedTime;
		if (state.buttons[GLFW_KEY_LEFT] == GLFW_PRESS)
		{
			xDiff -= buttonRotationSpeed;
		}
		if (state.buttons[GLFW_KEY_RIGHT] == GLFW_PRESS)
		{
			xDiff += buttonRotationSpeed;
		}
		if (state.buttons[GLFW_KEY_UP] == GLFW_PRESS)
		{
			yDiff += buttonRotationSpeed;
		}
		if (state.buttons[GLFW_KEY_DOWN] == GLFW_PRESS)
		{
			yDiff -= buttonRotationSpeed;
		}

		// Get Joystick Access states
		const XMFLOAT4 rightStick =
		{
			this->GetAxisInput(state, GLFW_GAMEPAD_AXIS_RIGHT_X),
			this->GetAxisInput(state, GLFW_GAMEPAD_AXIS_RIGHT_Y),
			0.0f,
			0.0f
		};

		const XMFLOAT4 leftStick =
		{
			this->GetAxisInput(state, GLFW_GAMEPAD_AXIS_LEFT_X),
			this->GetAxisInput(state, GLFW_GAMEPAD_AXIS_LEFT_Y),
			0.0f,
			0.0f
		};

		const float gamepadRotSpeed = 0.05f;
		xDiff += rightStick.x * gamepadRotSpeed;
		yDiff += rightStick.y * gamepadRotSpeed;

		// Process Translation
		bool const slowMode = state.buttons[GLFW_KEY_LEFT_SHIFT] == GLFW_PRESS || state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] == GLFW_PRESS;
		const float clampedDT = std::min(elapsedTime, 0.1f); // if dt > 100 millisec, don't allow the camera to jump too far...
		const float speed = (slowMode ? 1.0f : 10.0f) * 1.0f * clampedDT;

		static XMVECTOR prevMove = { 0.0f, 0.0f, 0.0f, 0.0f };
		XMVECTOR moveNew = XMVectorSet(leftStick.x, 0, -leftStick.y, 0);

		// TODO: handle keyboard

		moveNew *= speed;

		// TODO: Add Accelleration configuration
		const float acceleration = 0.03f;
		prevMove = XMVectorLerp(prevMove, moveNew, acceleration * clampedDT / 0.0166f); // smooth the movement a bit

		float moveLength = XMVectorGetX(XMVector3Length(prevMove));

		if (moveLength < 0.0001f)
		{
			prevMove = XMVectorSet(0, 0, 0, 0);
		}
		if (abs(xDiff) + abs(yDiff) > 0 || moveLength > 0.0001f)
		{
			XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&this->m_cameraTransform.LocalRotation));
			XMVECTOR moveRot = XMVector3TransformNormal(prevMove, camRot);
			XMFLOAT3 _move;
			XMStoreFloat3(&_move, moveRot);
			this->m_cameraTransform.Translate(_move);
			this->m_cameraTransform.RotateRollPitchYaw({ yDiff, xDiff, 0.0f });

			// TODO: Dirty Camera
			this->m_camera.IsDirty();
		}

		this->m_cameraTransform.UpdateTransform();

		this->m_camera.TransformCamera(this->m_cameraTransform);
		this->m_camera.UpdateCamera();
	}

private:
	float GetAxisInput(GLFWgamepadstate const& state, int inputId)
	{
		auto value = state.axes[inputId];
		return value > this->DeadZone || value < -this->DeadZone
			? value
			: 0.0f;
	}

private:
	const int m_joystickId = GLFW_JOYSTICK_1;
	const float DeadZone = 0.2f;

	float m_movementSpeed = 0.3f;
	float m_strafeMovementSpeed = 0.08f;
	float m_lookSpeed = 0.01f;

	float m_pitch = 0.0f;
	float m_yaw = 0.0f;
	CameraComponent& m_camera;
	TransformComponent m_cameraTransform;

	XMVECTOR m_worldUp;
	XMVECTOR m_worldNorth;
	XMVECTOR m_worldEast;
};

std::unique_ptr<ICameraController> PhxEngine::CreateDebugCameraController(PhxEngine::Renderer::CameraNode& camera, DirectX::XMVECTOR const& worldUp)
{
    return std::unique_ptr<ICameraController>();
}

std::unique_ptr<ICameraController> PhxEngine::CreateDebugCameraController(PhxEngine::Renderer::CameraComponent& camera, DirectX::XMVECTOR const& worldUp)
{
	return std::make_unique<DebugCameraController2>(camera, worldUp);
}
