#pragma once

#include <PhxEngine/Core/Primitives.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Graphics/RectPacker.h>
#include <DirectXMath.h>

namespace PhxEngine::Scene
{
	class Scene;
	struct LightComponent;
	struct CameraComponent;
}

namespace PhxEngine::Renderer
{

	constexpr static size_t kNumCascades = 3;
	class ShadowsAtlas
	{
	public:
		void Initialize(RHI::IGraphicsDevice* gfxDevice);
		void UpdatePerFrameData(RHI::ICommandList* commandList, Scene::Scene& scene, DirectX::XMFLOAT3 const& camPosition);

		RHI::TextureHandle Texture;
		RHI::RenderPassHandle ShadowRenderPass;
		// Helpers
		uint32_t Width = 0.0f;
		uint32_t Height = 0.0f;
		const RHI::RHIFormat Format = RHI::RHIFormat::D16;

	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		Graphics::RectPacker m_rectPack;
	};


	struct ShadowCam
	{
		DirectX::XMMATRIX ViewProjection;
		Core::Frustum Frustum;

		ShadowCam() = default;
		ShadowCam(DirectX::XMFLOAT3 const& eyePos, DirectX::XMFLOAT4 const& rotation, float nearPlane, float farPlane, float fov)
		{
			const DirectX::XMVECTOR eye = DirectX::XMLoadFloat3(&eyePos);
			const DirectX::XMVECTOR qRot = DirectX::XMQuaternionNormalize(DirectX::XMLoadFloat4(&rotation));
			const DirectX::XMMATRIX rot = DirectX::XMMatrixRotationQuaternion(qRot);
			const DirectX::XMVECTOR to = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), rot);
			const DirectX::XMVECTOR up = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rot);
			const DirectX::XMMATRIX V = DirectX::XMMatrixLookToRH(eye, to, up);

			// Note the farPlane is passed in as near, this is to support reverseZ
			const DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovRH(fov, 1, farPlane, nearPlane);
			this->ViewProjection = XMMatrixMultiply(V, P);
		}
	};

	void CreateDirectionLightShadowCams(
		Scene::CameraComponent const& cameraComponent,
		Scene::LightComponent& lightComponent,
		float maxZDepth,
		ShadowCam* shadowCams,
		bool reverseZ = false);

	void CreateSpotLightShadowCam(
		Scene::CameraComponent const& cameraComponent,
		Scene::LightComponent& lightComponent,
		ShadowCam& shadowCam,
		bool reverseZ = false);

	void CreateCubemapCameras(DirectX::XMFLOAT3 position, float nearZ, float farZ, ShadowCam* shadowCams);
}

