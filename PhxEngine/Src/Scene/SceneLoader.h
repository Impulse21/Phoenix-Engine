#pragma once

#include <string>
#include <memory>

#include "Graphics/RHI/PhxRHI.h"

namespace PhxEngine::Graphics
{
	class TextureCache;
}

namespace PhxEngine::Scene
{
	class Scene;
	class ISceneLoader
	{
	public:
		virtual bool LoadScene(std::string const& fileName, RHI::CommandListHandle commandList, Scene& scene) = 0;

		virtual ~ISceneLoader() = default;
	};

	std::unique_ptr<ISceneLoader> CreateGltfSceneLoader(
		RHI::IGraphicsDevice* graphicsDevice,
		std::shared_ptr<PhxEngine::Graphics::TextureCache> textureCache);
}