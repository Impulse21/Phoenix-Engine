#include "phxpch.h"

#include "SceneLoader.h"
#include "Scene.h"
#include "GltfSceneLoader.h"

using namespace PhxEngine::Scene;
using namespace PhxEngine::Core;
using namespace PhxEngine::Graphics;

std::unique_ptr<ISceneLoader> PhxEngine::Scene::CreateGltfSceneLoader(
	RHI::IGraphicsDevice* graphicsDevice,
	std::shared_ptr<TextureCache> textureCache)
{
	return std::make_unique<GltfSceneLoader>(graphicsDevice, textureCache);
}
