#pragma once

#include <string>
#include <memory>
#include <filesystem>

#include "PhxEngine/RHI/PhxRHI.h"

namespace PhxEngine::Core
{
	class IFileSystem;
}

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
		virtual bool LoadScene(
			std::shared_ptr<Core::IFileSystem> fileSystem,
			std::shared_ptr<Graphics::TextureCache> textureCache,
			std::filesystem::path const& fileName,
			RHI::ICommandList* commandList,
			PhxEngine::Scene::Scene& scene) = 0;

		virtual ~ISceneLoader() = default;
	};

	std::unique_ptr<ISceneLoader> CreateGltfSceneLoader();
}