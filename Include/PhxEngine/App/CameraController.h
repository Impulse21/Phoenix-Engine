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
	class ICameraController
	{
	public:
		virtual ~ICameraController() = default;

		virtual void Update(double elapsedTime) = 0;

		virtual void EnableDebugWindow(bool enabled) = 0;

	};

	std::unique_ptr<ICameraController> CreateDebugCameraController(
		PhxEngine::Renderer::CameraNode& camera,
		DirectX::XMVECTOR const& worldUp);

	std::unique_ptr<ICameraController> CreateDebugCameraController(
		PhxEngine::Renderer::CameraComponent& camera,
		DirectX::XMVECTOR const& worldUp);
}

