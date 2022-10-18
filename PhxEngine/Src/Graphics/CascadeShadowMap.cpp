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
	const DirectX::XMMATRIX viewProjectInv = DirectX::XMLoadFloat4x4(&cameraComponent.ViewProjectionInv);
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

	// The light view matrix, the UP cannot be parrell, and in the opposite direction as we wll get a zero vector, which fucks everything up. Need to determine based on rotation.
	const XMMATRIX vLightRotation = XMMatrixRotationQuaternion(XMLoadFloat4(&lightComponent.Rotation));
	const XMVECTOR to = XMVector3TransformNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), vLightRotation);
	const XMVECTOR up = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), vLightRotation);

	const DirectX::XMMATRIX lightView = 
		DirectX::XMMatrixLookToRH(
			DirectX::XMVectorZero(),
			to,
			up);

	// const float splitClamp = std::min(1.0f, (float)maxZDepth / cameraComponent.ZFar);
	// TODO: Apply a clamp to this ?
	float farSplit = 1.0f;
	float nearSplit = (farSplit / this->m_numCascades);

	std::vector<Renderer::RenderCam> retVal(this->m_numCascades);
	for (int i = this->m_numCascades - 1; i >= 0; i--)
	{
		if (i == 0)
		{
			nearSplit = 0;
		}

		// Adjust the frustrum corders to the split and move to light space.
		const DirectX::XMVECTOR cascadeCornersLS[] =
		{
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[0], frustumCornersWS[1], nearSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[0], frustumCornersWS[1], farSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[2], frustumCornersWS[3], nearSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[2], frustumCornersWS[3], farSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[4], frustumCornersWS[5], nearSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[4], frustumCornersWS[5], farSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[6], frustumCornersWS[7], nearSplit), lightView),
			DirectX::XMVector3Transform(DirectX::XMVectorLerp(frustumCornersWS[6], frustumCornersWS[7], farSplit), lightView),
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
			radius = std::max(radius, DirectX::XMVectorGetX(DirectX::XMVectorSubtract(cascadeCornersLS[i], cascadeCenterLS)));
		}

		DirectX::XMVECTOR vRadius = DirectX::XMVectorReplicate(radius);
		DirectX::XMVECTOR vMin = cascadeCenterLS - vRadius;
		DirectX::XMVECTOR vMax = cascadeCenterLS + vRadius;

		float resolution = RHI::IGraphicsDevice::Ptr->GetTextureDesc(this->m_shadowMapTexArray).Width;
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
				this->m_isReverseZ ? max.z : min.z,
				this->m_isReverseZ ? min.z : max.z);

		retVal[i] = {};
		retVal[i].ViewProjection = lightView * lightProjection;
	}

	return retVal;
}
