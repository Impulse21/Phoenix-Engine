#pragma once

#include <string>
#include <memory>

#include "PhxEngine/Graphics/RHI/PhxRHI.h"

namespace PhxEngine::Graphics
{
	class TextureCache;
}

namespace PhxEngine::Scene
{
	namespace Legacy
	{
		class Scene;
	}

	class ISceneLoader
	{
	public:
		virtual bool LoadScene(
			std::string const& fileName,
			RHI::CommandListHandle commandList,
			Legacy::Scene& scene) = 0;

		virtual ~ISceneLoader() = default;
	};

	namespace New
	{
		class Scene;

		class ISceneLoader
		{
		public:
			virtual bool LoadScene(
				std::string const& fileName,
				RHI::CommandListHandle commandList,
				New::Scene& scene) = 0;

			virtual ~ISceneLoader() = default;
		};
	}

	std::unique_ptr<ISceneLoader> CreateGltfSceneLoader(
		RHI::IGraphicsDevice* graphicsDevice,
		std::shared_ptr<PhxEngine::Graphics::TextureCache> textureCache);

	std::unique_ptr<New::ISceneLoader> CreateGltfSceneLoader();
}