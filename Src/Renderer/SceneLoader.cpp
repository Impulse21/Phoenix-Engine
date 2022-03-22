#include <PhxEngine/Renderer/SceneLoader.h>

#include <PhxEngine/Renderer/Scene.h>
#include <PhxEngine/Renderer/TextureCache.h>
#include <PhxEngine/Core/FileSystem.h>
#include "GltfSceneLoader.h"

using namespace PhxEngine::Renderer;
using namespace PhxEngine::Core;

std::unique_ptr<ISceneLoader> PhxEngine::Renderer::CreateGltfSceneLoader(
	std::shared_ptr<PhxEngine::Core::IFileSystem> fs,
	std::shared_ptr<PhxEngine::Renderer::TextureCache> textureCache,
	RHI::IGraphicsDevice* graphicsDevice)
{
	return std::make_unique<GltfSceneLoader>(fs, textureCache, graphicsDevice);
}
