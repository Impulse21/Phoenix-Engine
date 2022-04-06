#pragma once

#include <memory>
#include <DirectXMath.h>

namespace PhxEngine::Renderer
{
	class CameraNode;
	class CameraComponent;
}

namespace PhxEngine
{
	struct CameraControllerSettings
	{
		float MoveSpeed = 10.0f;
		float Acceleration = 0.018f;
	};

	class ICameraController
	{
	public:
		virtual ~ICameraController() = default;

		virtual void Update(float elapsedTime) = 0;

		virtual CameraControllerSettings& GetSettings() = 0;
	};

	std::unique_ptr<ICameraController> CreateDebugCameraController(
		PhxEngine::Renderer::CameraNode& camera,
		DirectX::XMVECTOR const& worldUp);

	std::unique_ptr<ICameraController> CreateDebugCameraController(
		PhxEngine::Renderer::CameraComponent& camera);
}

