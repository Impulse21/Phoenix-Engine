#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/ClusterLighting.h>

#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Core/Span.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Renderer;

namespace
{
	constexpr uint32_t kNumLights = 256;
	constexpr uint32_t kNumZBins = 16;
	constexpr uint32_t kTileSize = 8;
	constexpr uint32_t kNumWords = kNumLights + 31 / 32;

	struct SortedLight
	{
		uint32_t GlobalLightIndex;
		DirectX::XMVECTOR Position;
		float Range;
		float ProjectedZ;

		float ProjectedZMin;
		float ProjectedZMax;
	};
}

void PhxEngine::Renderer::ClusterLighting::Initialize(
	RHI::IGraphicsDevice* gfxDevice,
	DirectX::XMFLOAT2 const& canvas)
{
	this->SortedLightBuffer = gfxDevice->CreateBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Structured | RHI::BufferMiscFlags::Bindless,
			.Binding = RHI::BindingFlags::ShaderResource,
			.InitialState = RHI::ResourceStates::ShaderResource,
			.StrideInBytes = sizeof(uint32_t),
			.SizeInBytes = sizeof(uint32_t) * kNumLights,
			.DebugName = "Sorted Light Buffer"
		});
	this->LightLutBuffer = gfxDevice->CreateBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Structured | RHI::BufferMiscFlags::Bindless,
			.Binding = RHI::BindingFlags::ShaderResource,
			.InitialState = RHI::ResourceStates::ShaderResource,
			.StrideInBytes = sizeof(uint32_t),
			.SizeInBytes = sizeof(uint32_t) * kNumZBins,
			.DebugName = "Light LUT Buffer"
		});

	this->Canvas = canvas;
	this->TileXCount = canvas.x / kTileSize;
	this->TileYCount = canvas.y / kTileSize;
	this->TilesEntryCount = this->TileXCount * this->TileYCount * kNumWords;
	this->BufferSize = this->TilesEntryCount * sizeof(uint32_t);
	this->TileSizeInv = 1.0f / kTileSize;
	this->TileStride = this->TileXCount * kNumWords;

	this->LightTilesBuffer = gfxDevice->CreateBuffer({
			.MiscFlags = RHI::BufferMiscFlags::Structured | RHI::BufferMiscFlags::Bindless,
			.Binding = RHI::BindingFlags::ShaderResource,
			.InitialState = RHI::ResourceStates::ShaderResource,
			.StrideInBytes = sizeof(uint32_t),
			.SizeInBytes = BufferSize,
			.DebugName = "Light Tile Buffer"
		});
}

// Credit: Mastering-graphics-programming-with-vulkan
void PhxEngine::Renderer::ClusterLighting::Update(
	RHI::IGraphicsDevice* gfxDevice,
	RHI::ICommandList* commandList,
	Scene::Scene& scene,
	Scene::CameraComponent const& camera)
{
	// Move light updates to here?
	SortedLight sortedLights[kNumLights];

	// fill sorted light list
	auto lightView = scene.GetAllEntitiesWith<Scene::LightComponent>();
	uint32_t numLights = 0;

	// Fill data
	DirectX::XMMATRIX viewM = DirectX::XMLoadFloat4x4(&camera.View);
	DirectX::XMMATRIX projM = DirectX::XMLoadFloat4x4(&camera.Projection);
	for (auto e : lightView)
	{
		if (numLights >= kNumLights)
		{
			break;
		}

		auto& light = lightView.get<Scene::LightComponent>(e);

		DirectX::XMVECTOR positionV = DirectX::XMLoadFloat3(&light.Position);
		DirectX::XMVectorSetW(positionV, 1.0f);
		DirectX::XMVECTOR positionVS = DirectX::XMVector4Transform(positionV, viewM);

		DirectX::XMVECTOR positionMinVS = DirectX::XMVectorAdd(positionVS, DirectX::XMVectorSet(0.0f, 0.0f, -light.GetRange(), 0.0f));
		DirectX::XMVECTOR positionMaxVS = DirectX::XMVectorAdd(positionVS, DirectX::XMVectorSet(0.0f, 0.0f, light.GetRange(), 0.0f));

		SortedLight& sortedLight = sortedLights[numLights++];
		sortedLight.GlobalLightIndex = light.GlobalBufferIndex;
		sortedLight.Position = positionV;
		sortedLight.Range = light.GetRange();
		sortedLight.ProjectedZ = ((DirectX::XMVectorGetZ(positionVS) - camera.ZNear) / (camera.ZFar - camera.ZNear));
		sortedLight.ProjectedZMin = ((DirectX::XMVectorGetZ(positionMinVS) - camera.ZNear) / (camera.ZFar - camera.ZNear));
		sortedLight.ProjectedZMax = ((DirectX::XMVectorGetZ(positionMaxVS) - camera.ZNear) / (camera.ZFar - camera.ZNear));
	}

	std::sort(sortedLights, sortedLights + numLights, [](SortedLight& elemA, SortedLight const& elemB) {
			if (elemA.ProjectedZ < elemB.ProjectedZ)
			{
				return -1;
			}
			else if (elemA.ProjectedZ > elemB.ProjectedZ)
			{
				return 1;
			}
			return 0;
		});

	// Light list should already be uploaded to the GPU
	Core::Span<SortedLight> sortedLightSpan(sortedLights, numLights);

	// Bin slices
	float binSize = 1.0f / kNumZBins;

	RHI::GPUAllocation lightLutAllocation = commandList->AllocateGpu(sizeof(uint32_t) * kNumZBins, sizeof(uint32_t));
	uint32_t* pLightLut = static_cast<uint32_t*>(lightLutAllocation.CpuData);
	for (uint32_t iBin = 0; iBin < kNumZBins; iBin++)
	{
		uint32_t minLightId = kNumLights + 1;
		uint32_t maxLightId = 0;
		pLightLut[iBin] = minLightId | (maxLightId << 16u);

		float binMin = binSize * iBin;
		float binMax = binMin + binSize;

		for (int i = 0; i < sortedLightSpan.Size(); i++)
		{
			const SortedLight& light = sortedLightSpan[i];
			bool isInCurrentBin = (light.ProjectedZ >= binMin && light.ProjectedZ <= binMax)
				|| (light.ProjectedZMin >= binMin && light.ProjectedZMin <= binMax)
				|| (light.ProjectedZMax >= binMin && light.ProjectedZMax <= binMax);

			if (isInCurrentBin)
			{
				if (i < minLightId)
				{
					minLightId = i;
				}

				if (i > maxLightId)
				{
					maxLightId = i;
				}
			}
			// Set bin data. 
			pLightLut[iBin] = minLightId | (maxLightId << 16u);
		}
	}

	commandList->CopyBuffer(
		this->LightLutBuffer,
		0,
		lightLutAllocation.GpuBuffer,
		lightLutAllocation.Offset,
		lightLutAllocation.SizeInBytes);

	RHI::GPUAllocation lightIndices = commandList->AllocateGpu(sizeof(uint32_t) * kNumLights, sizeof(uint32_t));
	uint32_t* pLightIndices = static_cast<uint32_t*>(lightLutAllocation.CpuData);
	for (uint32_t i = 0; i < sortedLightSpan.Size(); ++i)
	{
		pLightIndices[i] = sortedLightSpan[i].GlobalLightIndex;
	}
	commandList->CopyBuffer(
		this->SortedLightBuffer,
		0,
		lightIndices.GpuBuffer,
		lightIndices.Offset,
		lightIndices.SizeInBytes);

	const uint32_t tileXCount = this->TileXCount;
	const uint32_t tileYCount = this->TileYCount;
	const uint32_t tilesEntryCount = this->TilesEntryCount;
	const uint32_t bufferSize = this->BufferSize;
	const float tileSizeInv = this->TileSizeInv;
	const uint32_t tileStride = this->TileStride;


	RHI::GPUAllocation lightTileBits = commandList->AllocateGpu(bufferSize, sizeof(uint32_t));
	uint32_t* pLightTileBits = static_cast<uint32_t*>(lightTileBits.CpuData);
	std::memset(pLightTileBits, 0, bufferSize);
	for (uint32_t i = 0; i < sortedLightSpan.Size(); ++i)
	{
		const SortedLight& light = sortedLightSpan[i];
		// Calculate light AABB
		
		DirectX::XMFLOAT4 lightBoundingVS = {};
		DirectX::XMFLOAT2 minBoundVS = { FLT_MAX, FLT_MAX };
		DirectX::XMFLOAT2 maxBoundVS = { -FLT_MAX, -FLT_MAX };

		// Project bounding box into clip space
		for (int c = 0; c < 8; c++)
		{
			DirectX::XMVECTOR corner = DirectX::XMVectorSet(
				(c % 2) ? 1.0f : -1.0f,
				(c & 2) ? 1.0f : -1.0f,
				(c & 4) ? 1.0f : -1.0f,
				1.0f);

			corner = DirectX::XMVectorScale(corner, light.Range);
			corner = DirectX::XMVectorAdd(corner, light.Position);
			DirectX::XMVectorSetW(corner, 1.0f);

			// Transform into View space
			DirectX::XMVECTOR cornerVS = DirectX::XMVector4Transform(corner, viewM);

			// adjust z on the near plane.
			// visible Z is negative, thus corner vs will be always negative, but near is positive.
			// get positive Z and invert ad the end.
			DirectX::XMVectorSetZ(cornerVS, std::max(camera.ZNear, DirectX::XMVectorGetZ(cornerVS)));

			DirectX::XMVECTOR cornerNDCV = DirectX::XMVector3TransformCoord(cornerVS, projM);
			DirectX::XMFLOAT4 cornerNDC;
			DirectX::XMStoreFloat4(&cornerNDC, cornerNDCV);
			minBoundVS.x = std::min(minBoundVS.x, cornerNDC.x);
			minBoundVS.y = std::min(minBoundVS.y, cornerNDC.y);

			maxBoundVS.x = std::max(maxBoundVS.x, cornerNDC.x);
			maxBoundVS.y = std::max(maxBoundVS.y, cornerNDC.y);
		}

		lightBoundingVS.x = minBoundVS.x;
		lightBoundingVS.z = maxBoundVS.x;

		// Inverted Y aabb
		lightBoundingVS.w = -1 * minBoundVS.y;
		lightBoundingVS.y = -1 * maxBoundVS.y;
		// Collect light Data.

		float positionLen = DirectX::XMVectorGetZ(DirectX::XMVector3Length(light.Position));
		const bool cameraInside = (positionLen - light.Range) < camera.ZFar;

		DirectX::XMFLOAT4 aabbScreen ={
			(lightBoundingVS.x * 0.5f + 0.5f) * (this->Canvas.x - 1),
			(lightBoundingVS.y * 0.5f + 0.5f) * (this->Canvas.y - 1),
			(lightBoundingVS.z * 0.5f + 0.5f) * (this->Canvas.x - 1),
			(lightBoundingVS.w * 0.5f + 0.5f) * (this->Canvas.y - 1) };

		float width = aabbScreen.z - aabbScreen.x;
		float height = aabbScreen.w - aabbScreen.y;

		if (width < 0.0001f || height < 0.0001f) 
		{
			continue;
		}

		float minX = aabbScreen.x;
		float minY = aabbScreen.y;

		float maxX = minX + width;
		float maxY = minY + height;

		if (minX > this->Canvas.x || minY > this->Canvas.y)
		{
			continue;
		}

		if (maxX < 0.0f || maxY < 0.0f)
		{
			continue;
		}

		minX = std::max(minX, 0.0f);
		minY = std::max(minY, 0.0f);

		maxX = std::min(maxX, (float)this->Canvas.x);
		maxY = std::min(maxY, (float)this->Canvas.y);

		uint32_t firstTileX = (uint32_t)(minX * tileSizeInv);
		uint32_t lastTileX = std::min(tileXCount - 1, (uint32_t)(maxX * tileSizeInv));

		uint32_t firstTileY = (uint32_t)(minY * tileSizeInv);
		uint32_t lastTileY = std::min(tileYCount - 1, (uint32_t)(maxY * tileSizeInv));
		
		for (uint32_t y = firstTileY; y <= lastTileY; ++y)
		{
			for (uint32_t x = firstTileX; x <= lastTileX; ++x)
			{
				uint32_t arrayIndex = y * tileStride + x;

				uint32_t wordIndex = i / 32;
				uint32_t bitIndex = i % 32;

				pLightTileBits[arrayIndex + wordIndex] |= (1 << bitIndex);
			}
		}
	}

	commandList->CopyBuffer(
		this->LightTilesBuffer,
		0,
		lightTileBits.GpuBuffer,
		lightTileBits.Offset,
		lightTileBits.SizeInBytes);
}
