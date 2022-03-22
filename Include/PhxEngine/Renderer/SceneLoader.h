#pragma once

#include <string>
#include <memory>

#include <PhxEngine/RHI/PhxRHI.h>

namespace PhxEngine
{

	namespace Core
	{
		class IFileSystem;
	}
}

namespace PhxEngine::Renderer
{
	class TextureCache;

	namespace New
	{
		class Scene;
	}

	class ISceneLoader
	{
	public:
		virtual bool LoadScene(std::string const& fileName, PhxEngine::RHI::CommandListHandle commandList, New::Scene& scene) = 0;

		virtual ~ISceneLoader() = default;
	};

	std::unique_ptr<ISceneLoader> CreateGltfSceneLoader(
		std::shared_ptr<PhxEngine::Core::IFileSystem> fs,
		std::shared_ptr<PhxEngine::Renderer::TextureCache> textureCache,
		PhxEngine::RHI::IGraphicsDevice* graphicsDevice);
}