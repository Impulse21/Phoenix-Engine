#pragma once
#include <stdint.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <DirectXMath.h>

namespace PhxEngine::Scene
{
	class Scene;
	struct CameraComponent;
}

namespace PhxEngine::Renderer
{
	class ClusterLighting
	{
	public:
		void Initialize(
			RHI::IGraphicsDevice* gfxDevice,
			DirectX::XMFLOAT2 const& canvas);

		void Update(
			RHI::IGraphicsDevice* gfxDevice,
			RHI::ICommandList* commandList,
			Scene::Scene& scene,
			Scene::CameraComponent const& camera);

		RHI::BufferHandle SortedLightBuffer;
		RHI::BufferHandle LightLutBuffer;
		RHI::BufferHandle LightTilesBuffer;

		DirectX::XMFLOAT2 Canvas;
		uint32_t TileXCount;
		uint32_t TileYCount;
		uint32_t TilesEntryCount;
		uint32_t BufferSize;
		float TileSizeInv;
		uint32_t TileStride;
	};
}

