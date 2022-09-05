#include "phxpch.h"

#include "PhxEngine/Scene/SceneLoader.h"
#include "PhxEngine/Scene/Scene.h"
#include "GltfSceneLoader.h"

using namespace PhxEngine::Scene;
using namespace PhxEngine::Core;
using namespace PhxEngine::Graphics;

std::unique_ptr<ISceneLoader> PhxEngine::Scene::CreateGltfSceneLoader(
	RHI::IGraphicsDevice* graphicsDevice,
	std::shared_ptr<TextureCache> textureCache)
{
	return nullptr;
}

std::unique_ptr<New::ISceneLoader> PhxEngine::Scene::CreateGltfSceneLoader()
{
	return std::make_unique<GltfSceneLoader>();
}