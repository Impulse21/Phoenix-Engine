#pragma once

#include <DirectXMath.h>
#include <PhxEngine/Core/Span.h>

namespace PhxEngine::Renderer
{

	struct RenderCam
	{
		DirectX::XMMATRIX ViewProjection;

		RenderCam() = default;
		RenderCam(DirectX::XMFLOAT3 const& eyePos, DirectX::XMFLOAT4 const& rotation, float nearPlane, float farPlane, float fov)
		{
			const DirectX::XMVECTOR eye = DirectX::XMLoadFloat3(&eyePos);
			const DirectX::XMVECTOR qRot = DirectX::XMQuaternionNormalize(DirectX::XMLoadFloat4(&rotation));
			const DirectX::XMMATRIX rot = DirectX::XMMatrixRotationQuaternion(qRot);
			const DirectX::XMVECTOR to = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), rot);
			const DirectX::XMVECTOR up = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rot);
			const DirectX::XMMATRIX V = DirectX::XMMatrixLookToRH(eye, to, up);
			const DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovRH(fov, 1, farPlane, nearPlane);
			this->ViewProjection = XMMatrixMultiply(V, P);
		}
	};

	void CreateCubemapCameras(DirectX::XMFLOAT3 position, float nearZ, float farZ, Core::Span<RenderCam> cameras);

	class Renderer
	{
	public:
		// Global pointer set by intializer
		inline static Renderer* Ptr = nullptr;

	public:
		virtual ~Renderer() = default;
	};
}