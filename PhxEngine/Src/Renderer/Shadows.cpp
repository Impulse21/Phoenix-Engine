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

		const float dist = Math::Distance(camPosition, lightComponent.Position);
		const float range = lightComponent.Range;
		const float amount = std::min(1.0f, range / std::max(0.001f, dist));

		PackerRect rect = {};
		rect.id = (int)e;
		switch (lightComponent.Type)
		{
		case Scene::LightComponent::kDirectionalLight:
			rect.w = kMaxShadowRes_2D * kNumCascades;
			rect.h = kMaxShadowRes_2D * kNumCascades;
			break;
		case Scene::LightComponent::kOmniLight:
			rect.w = int(kMaxShadowRes_2D * amount);
			rect.h = int(kMaxShadowRes_2D * amount);
			break;
		case Scene::LightComponent::kSpotLight:
			rect.w = int(kMaxShadowRes_Cube * amount) * 6;
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
			case Scene::LightComponent::kSpotLight:
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
	float maxZDepth,
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
	const XMMATRIX vLightRotation = XMMatrixRotationQuaternion(XMLoadFloat4(&lightComponent.Rotation));
	const XMVECTOR to = XMVector3TransformNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), vLightRotation);
	const XMVECTOR up = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), vLightRotation);

	const DirectX::XMMATRIX lightView =
		DirectX::XMMatrixLookToRH(
			DirectX::XMVectorZero(),
			to,
			up);


	float splitDepthClamp = std::min(1.0f, maxZDepth / cameraComponent.ZFar);

	std::array<float, kNumCascades + 1> cascadeSplits
	{
		splitDepthClamp * 0.0f,		// near plane
		splitDepthClamp * 0.01f,	// near-mid split
		splitDepthClamp * 0.1f,		// mid-far split
		splitDepthClamp * 1.0f,		// far plane
	};

	// const float splitClamp = std::min(1.0f, (float)maxZDepth / cameraComponent.ZFar);
	// TODO: Apply a clamp to this ?

	for (int i = 0; i < kNumCascades; i++)
	{
		float nearSplit = cascadeSplits[i];
		float farSplit = cascadeSplits[i + 1];

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
			DirectX::XMMatrixOrthographicOffCenterRH(
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
	shadowCam = ShadowCam(lightComponent.Position, lightComponent.Rotation, 0.1f, lightComponent.Range, lightComponent.OuterConeAngle * 2);
	shadowCam.Frustum = Core::Frustum(shadowCam.ViewProjection);
}
