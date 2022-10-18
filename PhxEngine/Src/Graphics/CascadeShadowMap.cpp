#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "PhxEngine/Graphics/CascadeShadowMap.h"

#include <DirectXMath.h>

using namespace PhxEngine;
using namespace PhxEngine::Graphics;

CascadeShadowMap::CascadeShadowMap(uint32_t resolution, uint16_t numCascades, RHI::FormatType format, bool isReverseZ)
	: m_isReverseZ(isReverseZ)
	, m_numCascades(numCascades)
{
	assert(numCascades > 0);

	this->m_shadowMapTexArray = RHI::IGraphicsDevice::Ptr->CreateTexture(
		{
			.BindingFlags = RHI::BindingFlags::DepthStencil | RHI::BindingFlags::ShaderResource,
			.Dimension = RHI::TextureDimension::Texture2DArray,
			.InitialState = RHI::ResourceStates::DepthWrite,
			.Format = format,
			.Width = resolution,
			.Height = resolution,
			.ArraySize = numCascades,
			.OptmizedClearValue = { .DepthStencil = {.Depth = this->m_isReverseZ ? 0.0f : 1.0f } },
			.DebugName = "Cascade Shadow maps",
		});

	this->m_renderPass = RHI::IGraphicsDevice::Ptr->CreateRenderPass(
		{
			.Attachments =
			{
				{
					.Type = RHI::RenderPassAttachment::Type::DepthStencil,
					.LoadOp = RHI::RenderPassAttachment::LoadOpType::Clear,
					.Texture = this->m_shadowMapTexArray,
					.StoreOp = RHI::RenderPassAttachment::StoreOpType::Store,
					.InitialLayout = RHI::ResourceStates::DepthWrite,
					.SubpassLayout = RHI::ResourceStates::DepthWrite,
					.FinalLayout = RHI::ResourceStates::DepthWrite
				},
			}
		});
}

CascadeShadowMap::~CascadeShadowMap()
{
	if (this->m_renderPass.IsValid())
	{
		RHI::IGraphicsDevice::Ptr->DeleteRenderPass(this->m_renderPass);
	}

	if (this->m_shadowMapTexArray.IsValid())
	{
		RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_shadowMapTexArray);
	}
}

std::vector<Renderer::RenderCam> PhxEngine::Graphics::CascadeShadowMap::CreateRenderCams(
	Scene::CameraComponent const& cameraComponent,
	Scene::LightComponent& lightComponent,
	uint32_t maxZDepth)
{
	// Construct a frustrum corders from an NDC Matrix, if reverse Z, then swap the Z
	const float ndcZNear = this->m_isReverseZ ? 1.0f : 0.0f;
	const float ndcZFar = this->m_isReverseZ ? 0.0f : 1.0f;
	const DirectX::XMVECTOR vLightDir = DirectX::XMLoadFloat3(&lightComponent.Direction);
	const DirectX::XMMATRIX viewProjectInv = DirectX::XMLoadFloat4x4(&cameraComponent.ViewProjectionInv);
	// const DirectX::XMMATRIX lightViewWicked = DirectX::XMMatrixLookToRH(DirectX::XMVectorZero(), vLightDir, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f));

	// Experiment pulling this info directly from the view projection
	const std::array<DirectX::XMVECTOR, 8> frustumCornersWS =
	{
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(-1, -1, ndcZNear, 1.0f),	viewProjectInv), // Near
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(-1, -1, ndcZFar, 1.0f),	viewProjectInv),// Far
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(-1, 1, ndcZNear, 1.0f),	viewProjectInv),// Near
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(-1, 1, ndcZFar, 1.0f),	viewProjectInv), // Far
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1, -1, ndcZNear, 1.0f),	viewProjectInv), // Near
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1, -1, ndcZFar, 1.0f),	viewProjectInv), // Far
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1, 1, ndcZNear, 1.0f),	viewProjectInv), // Near
		DirectX::XMVector3TransformCoord(DirectX::XMVectorSet(1, 1, ndcZFar, 1.0f),		viewProjectInv), // Far
	};

	// TODO: Apply a clamp to this ?
	float farSplit = 1.0f;
	float nearSplit = farSplit / this->m_numCascades;
	std::vector<Renderer::RenderCam> retVal(this->m_numCascades);
	for (int i = this->m_numCascades - 1; i >= 0; i--)
	{
		if (i == 0)
		{
			nearSplit = 0;
		}

		// Adjust the frustrum corders to the split and move to light space.
		const DirectX::XMVECTOR cascadeCornersWS[] =
		{
			DirectX::XMVectorLerp(frustumCornersWS[0], frustumCornersWS[1], nearSplit),
			DirectX::XMVectorLerp(frustumCornersWS[0], frustumCornersWS[1], farSplit),
			DirectX::XMVectorLerp(frustumCornersWS[2], frustumCornersWS[3], nearSplit),
			DirectX::XMVectorLerp(frustumCornersWS[2], frustumCornersWS[3], farSplit),
			DirectX::XMVectorLerp(frustumCornersWS[4], frustumCornersWS[5], nearSplit),
			DirectX::XMVectorLerp(frustumCornersWS[4], frustumCornersWS[5], farSplit),
			DirectX::XMVectorLerp(frustumCornersWS[6], frustumCornersWS[7], nearSplit),
			DirectX::XMVectorLerp(frustumCornersWS[6], frustumCornersWS[7], farSplit),
		};

		const int numCorners = ARRAYSIZE(cascadeCornersWS);

		// Calculate the Center of the boxes by taking the average of all the points
		XMVECTOR centerWS = DirectX::XMVectorZero();
		for (int j = 0; j < numCorners; ++j)
		{
			centerWS = DirectX::XMVectorAdd(centerWS, cascadeCornersWS[j]);
		}

		centerWS = centerWS / (float)numCorners;

#define EnableBoundingSphere 0
#if EnableBoundingSphere
		// Compute radius of bounding sphere
		float radius = 0.0f;
		for (int j = 0; j < numCorners; ++j)
		{
			radius = std::max(radius, DirectX::XMVectorGetX(DirectX::XMVectorSubtract(cascadeCornersWS[i], centerWS)));
		}

		DirectX::XMVECTOR centerLS = DirectX::XMVector2Transform(centerWS, lightView);

		// Move Center into light view
		DirectX::XMVECTOR vRadius = DirectX::XMVectorReplicate(radius);
		DirectX::XMVECTOR vMin = centerLS - vRadius;
		DirectX::XMVECTOR vMax = centerLS + vRadius;
#else
		DirectX::XMMATRIX lightView = DirectX::XMMatrixLookToRH(
			centerWS,
			vLightDir,
			DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f));

		DirectX::XMVECTOR vMin = DirectX::XMVectorSet(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1.0f);
		DirectX::XMVECTOR vMax = DirectX::XMVectorSet(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), 1.0f);;
		for (const auto& corner : cascadeCornersWS)
		{
			const DirectX::XMVECTOR cornerLS = DirectX::XMVector3Transform(corner, lightView);
			vMin = DirectX::XMVectorMin(vMin, cornerLS);
			vMax = DirectX::XMVectorMax(vMax, cornerLS);
		}


		DirectX::XMFLOAT3 min = {};
		DirectX::XMStoreFloat3(&min, vMin);

		DirectX::XMFLOAT3 max = {};
		DirectX::XMStoreFloat3(&max, vMax);

		float resolution = RHI::IGraphicsDevice::Ptr->GetTextureDesc(this->m_shadowMapTexArray).Width;

		// Snap cascade to texel grid:
		const XMVECTOR extent = XMVectorSubtract(vMax, vMin);
		const XMVECTOR texelSize = extent / float(resolution);
		vMin = XMVectorFloor(vMin / texelSize) * texelSize;
		vMax = XMVectorFloor(vMax / texelSize) * texelSize;

		// Extend Z to avoid early z cliping
		// Tune this parameter according to the scene
		constexpr float zMult = 10.0f;
		min.z = min.z < 0 ? min.z * zMult : min.z / zMult;
		max.z = max.z < 0 ? max.z / zMult : max.z * zMult;

		const DirectX::XMMATRIX lightProjection = 
			DirectX::XMMatrixOrthographicOffCenterLH(
				min.x,
				max.x,
				min.y,
				max.y,
				this->m_isReverseZ ? max.z : min.z,
				this->m_isReverseZ ? min.z : max.z);
#endif
		retVal[i] = {};
		retVal[i].ViewProjection = lightView * lightProjection;
	}

	return retVal;
}
