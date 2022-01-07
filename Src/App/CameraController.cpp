#include <PhxEngine/App/CameraController.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Renderer/SceneNodes.h>

#include <GLFW/glfw3.h>

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
			LOG_ERROR("Unable to locate joystick. This is currently required by the application");
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
		this->ShowDebugWindow();

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

	void ShowDebugWindow()
	{
		/*
		ImGui::Begin("Camera Info", &this->m_enableDebugWindow, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Controller Type:  Debug Camera");
		ImGui::Text("Pitch %f (Degrees)", XMConvertToDegrees(this->m_pitch));
		ImGui::Text("Yaw %f (Degrees)", XMConvertToDegrees(this->m_yaw));
		ImGui::NewLine();

		ImGui::DragFloat("Movement Speed", &this->m_movementSpeed, 0.001f, 0.001f, 1.0f);
		ImGui::DragFloat("Strafe Speed", &this->m_strafeMovementSpeed, 0.001f, 0.001f, 1.0f);
		ImGui::DragFloat("Look Speed", &this->m_lookSpeed, 0.001f, 0.001f, 1.0f);

		ImGui::End();
		*/
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

std::unique_ptr<ICameraController> PhxEngine::CreateDebugCameraController(PhxEngine::Renderer::CameraNode& camera, DirectX::XMVECTOR const& worldUp)
{
    return std::unique_ptr<ICameraController>();
}
