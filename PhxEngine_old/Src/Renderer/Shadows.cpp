#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/Shadows.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Core/Math.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::Graphics;

namespace
{
	constexpr static int kMaxShadowRes_2D = 1024;
	constexpr static int kMaxShadowRes_Cube = 256;
	constexpr static int kNumCascades = 3;
	constexpr static int kMaxAtlaisWidth = 8192;
}

void PhxEngine::Renderer::ShadowsAtlas::Initialize(RHI::IGraphicsDevice* gfxDevice)
{
	this->m_gfxDevice = gfxDevice;
}

// Credit Wicked Engine https://github.com/turanszkij/WickedEngine
void PhxEngine::Renderer::ShadowsAtlas::UpdatePerFrameData(RHI::ICommandList* commandList, Scene::Scene& scene, DirectX::XMFLOAT3 const& camPosition)
{
	// TODO:: Bench mark
	if (scene.GetShaderData().LightCount == 0)
	{
		return;
	}

	this->m_rectPack.Clear();

	auto lightView = scene.GetAllEntitiesWith<Scene::LightComponent>();
	for (auto e : lightView)
	{
		auto lightComponent = lightView.get<Scene::LightComponent>(e);
		if (!lightComponent.IsEnabled() || !lightComponent.CastShadows())
		{
			continue;
		}

		const float dist = Math::Distance(camPosition, lightComponent.Position);
		const float range = lightComponent.GetRange();
		const float amount = std::min(1.0f, range / std::max(0.001f, dist));

		PackerRect rect = {};
		rect.id = (int)e;
		switch (lightComponent.Type)
		{
		case Scene::LightComponent::kDirectionalLight:
			rect.w = kMaxShadowRes_2D * kNumCascades;
			rect.h = kMaxShadowRes_2D;
			break;
		case Scene::LightComponent::kOmniLight:
			rect.w = int(kMaxShadowRes_2D * amount) * 6;
			rect.h = int(kMaxShadowRes_2D * amount);
			break;
		case Scene::LightComponent::kSpotLight:
			rect.w = int(kMaxShadowRes_Cube * amount);
			rect.h = int(kMaxShadowRes_Cube * amount);
			break;
		}
		
		if (rect.w > 8 && rect.h > 8)
		{
			this->m_rectPack.AddRect(rect);
		}
	}

	if (this->m_rectPack.IsEmpty())
	{
		return;
	}

	if (this->m_rectPack.Pack(kMaxAtlaisWidth))
	{
		for (auto& rect : this->m_rectPack.GetRects())
		{
			entt::entity e = (entt::entity)rect.id;
			auto& lightComponent = lightView.get<Scene::LightComponent>(e);

			if (!rect.was_packed)
			{
				lightComponent.Direction = {};
			}

			lightComponent.ShadowRect = rect;
			switch (lightComponent.Type)
			{
			case Scene::LightComponent::kDirectionalLight:
				lightComponent.ShadowRect.w /= kNumCascades;
				break;
			case Scene::LightComponent::kOmniLight:
				lightComponent.ShadowRect.w /= 6;
				break;
			}
		}

		if (!this->Texture.IsValid() ||
			this->Width < (uint32_t)this->m_rectPack.GetWidth() ||
			this->Height < (uint32_t)this->m_rectPack.GetHeight())
		{
			if (this->Texture.IsValid())
			{
				this->m_gfxDevice->DeleteTexture(this->Texture);
			}

			this->Width = (uint32_t)this->m_rectPack.GetWidth();
			this->Height = (uint32_t)this->m_rectPack.GetHeight();

			this->Texture = this->m_gfxDevice->CreateTexture({
					.BindingFlags = RHI::BindingFlags::DepthStencil | RHI::BindingFlags::ShaderResource,
					.InitialState = RHI::ResourceStates::ShaderResource,
					.Format = this->Format,
					.Width = (uint32_t)this->Width,
					.Height = (uint32_t)this->Height,
					.OptmizedClearValue = { .DepthStencil = 1.0f, },
					.DebugName = "Shadow Atlas"
				});


			if (this->ShadowRenderPass.IsValid())
			{
				this->m_gfxDevice->DeleteRenderPass(this->ShadowRenderPass);
			}
			this->ShadowRenderPass = this->m_gfxDevice->CreateRenderPass(
				{
					.Attachments =
					{
						{
							.Type = RHI::RenderPassAttachment::Type::DepthStencil,
							.LoadOp = RHI::RenderPassAttachment::LoadOpType::Clear,
							.Texture = this->Texture,
							.InitialLayout = RHI::ResourceStates::ShaderResource,
							.SubpassLayout = RHI::ResourceStates::DepthWrite,
							.FinalLayout = RHI::ResourceStates::ShaderResource
						},
					}
				});
		}
	}
}

void PhxEngine::Renderer::CreateDirectionLightShadowCams(
	Scene::CameraComponent const& cameraComponent,
	Scene::LightComponent& lightComponent,
	ShadowCam* shadowCams,
	bool reverseZ)
{

	// Construct a frustrum corders from an NDC Matrix, if reverse Z, then swap the Z
	const float ndcZNear = reverseZ ? 1.0f : 0.0f;
	const float ndcZFar = reverseZ ? 0.0f : 1.0f;
	const DirectX::XMMATRIX viewProjectInv = DirectX::XMLoadFloat4x4(&cameraComponent.ViewProjectionInv);

	// Experiment pulling this info directly from the view projection
	const std::array<DirectX::XMVECTOR, 8> frustumCornersWS =
	{
		// Near
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1, 1, ndcZNear, 1.0f),	viewProjectInv),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1, -1, ndcZNear, 1.0f),	viewProjectInv),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(-1, 1, ndcZNear, 1.0f),	viewProjectInv),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(-1, -1, ndcZNear, 1.0f),	viewProjectInv),

		// Far
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1, 1, ndcZFar, 1.0f),		viewProjectInv),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1, -1, ndcZFar, 1.0f),	viewProjectInv),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(-1, 1, ndcZFar, 1.0f),	viewProjectInv),
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(-1, -1, ndcZFar, 1.0f),	viewProjectInv),
	};

	// The light view matrix, the UP cannot be parrell, and in the opposite direction as we wll get a zero vector, which fucks everything up. Need to determine based on rotation.
	// TODO Fix-me: Added a little hack to correct the y axis on the quaternion - I don't know why this is required and I need to do more digging. I suspect the loader isn't loading this data correctly
	// const XMMATRIX vLightRotation = XMMatrixRotationQuaternion(XMLoadFloat4(&lightComponent.Rotation));
	const XMMATRIX vLightRotation = XMMatrixRotationQuaternion(XMLoadFloat4(&lightComponent.Rotation) * XMVectorSet(1.0f, -1.0f, 1.0f, 1.0f));
	const XMVECTOR to = XMVector3TransformNormal(XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f), vLightRotation);
	const XMVECTOR up = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), vLightRotation);

	const DirectX::XMMATRIX lightView =
		DirectX::XMMatrixLookToLH(
			DirectX::XMVectorZero(),
			to,
			up);


	const float farPlane = cameraComponent.ZFar;

	// TODO: Make this configurable.
	std::array<float, kNumCascades> cascadeDistances =
	{
		20,
		120,
		1200
	};

	// const float splitClamp = std::min(1.0f, (float)maxZDepth / cameraComponent.ZFar);
	// TODO: Apply a clamp to this ?

	for (int i = 0; i < kNumCascades; i++)
	{
		const float nearSplit = i == 0 ? 0.0f : cascadeDistances[i - 1] / farPlane;
		const float farSplit = cascadeDistances[i] / farPlane;

		// Adjust the frustrum corders to the split and move to light space.
		const DirectX::XMVECTOR cascadeCornersLS[] =
		{
			// Near
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[0], frustumCornersWS[4], nearSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[1], frustumCornersWS[5], nearSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[2], frustumCornersWS[6], nearSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[3], frustumCornersWS[7], nearSplit), lightView),

			// Far
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[0], frustumCornersWS[4], farSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[1], frustumCornersWS[5], farSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[2], frustumCornersWS[6], farSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[3], frustumCornersWS[7], farSplit), lightView),
		};

		const int numCorners = ARRAYSIZE(cascadeCornersLS);

		// Calculate the Center of the boxes by taking the average of all the points
		XMVECTOR cascadeCenterLS = DirectX::XMVectorZero();
		for (int j = 0; j < numCorners; ++j)
		{
			cascadeCenterLS = DirectX::XMVectorAdd(cascadeCenterLS, cascadeCornersLS[j]);
		}

		cascadeCenterLS = cascadeCenterLS / (float)numCorners;// Compute radius of bounding sphere

		float radius = 0.0f;
		for (int j = 0; j < numCorners; ++j)
		{
			radius = std::max(radius, DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(cascadeCornersLS[i], cascadeCenterLS))));
		}

		DirectX::XMVECTOR vRadius = DirectX::XMVectorReplicate(radius);
		DirectX::XMVECTOR vMin = cascadeCenterLS - vRadius;
		DirectX::XMVECTOR vMax = cascadeCenterLS + vRadius;

		float resolution = lightComponent.ShadowRect.w;
		const XMVECTOR extent = XMVectorSubtract(vMax, vMin);
		const XMVECTOR texelSize = extent / float(resolution);
		vMin = XMVectorFloor(vMin / texelSize) * texelSize;
		vMax = XMVectorFloor(vMax / texelSize) * texelSize;
		cascadeCenterLS = (vMin + vMax) * 0.5f;


		DirectX::XMFLOAT3 min = {};
		DirectX::XMStoreFloat3(&min, vMin);

		DirectX::XMFLOAT3 max = {};
		DirectX::XMStoreFloat3(&max, vMax);

		DirectX::XMFLOAT3 center = {};
		DirectX::XMStoreFloat3(&center, cascadeCenterLS);

		// Extrude bounds to avoid early shadow clipping:
		float ext = abs(center.z - min.z);
		ext = std::max(ext, std::min(1500.0f, cameraComponent.ZFar) * 0.5f);
		min.z = center.z - ext;
		max.z = center.z + ext;


		const DirectX::XMMATRIX lightProjection =
			DirectX::XMMatrixOrthographicOffCenterLH(
				min.x,
				max.x,
				min.y,
				max.y,
				reverseZ ? max.z : min.z,
				reverseZ ? min.z : max.z);

		shadowCams[i] = {};
		shadowCams[i].ViewProjection = lightView * lightProjection;
		shadowCams[i].Frustum = Core::Frustum(shadowCams[i].ViewProjection);
	}
}

void PhxEngine::Renderer::CreateSpotLightShadowCam(
	Scene::CameraComponent const& cameraComponent,
	Scene::LightComponent& lightComponent,
	ShadowCam& shadowCam,
	bool reverseZ)
{
	shadowCam = ShadowCam(lightComponent.Position, lightComponent.Rotation, 0.1f, lightComponent.GetRange(), lightComponent.OuterConeAngle * 2);
	shadowCam.Frustum = Core::Frustum(shadowCam.ViewProjection);
}

void PhxEngine::Renderer::CreateCubemapCameras(DirectX::XMFLOAT3 position, float nearZ, float farZ, ShadowCam* shadowCams)
{
	shadowCams[0] = ShadowCam(position, DirectX::XMFLOAT4(0.5f, -0.5f, -0.5f, -0.5f), nearZ, farZ, DirectX::XM_PIDIV2); // +x
	shadowCams[1] = ShadowCam(position, DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, -0.5f), nearZ, farZ, DirectX::XM_PIDIV2); // -x
	shadowCams[2] = ShadowCam(position, DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f), nearZ, farZ, DirectX::XM_PIDIV2); // +y
	shadowCams[3] = ShadowCam(position, DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, -1.0f), nearZ, farZ, DirectX::XM_PIDIV2); // -y
	shadowCams[4] = ShadowCam(position, DirectX::XMFLOAT4(0.707f, 0.0f, 0.0f, -0.707f), nearZ, farZ, DirectX::XM_PIDIV2); // +z
	shadowCams[5] = ShadowCam(position, DirectX::XMFLOAT4(0.0f, 0.707f, 0.707f, 0.0f), nearZ, farZ, DirectX::XM_PIDIV2); // -z
}