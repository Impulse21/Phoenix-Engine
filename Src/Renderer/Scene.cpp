#include "..\..\Include\PhxEngine\Renderer\Scene.h"

PhxEngine::Renderer::SceneGraph::SceneGraph()
{
	this->m_rootNode = std::make_shared<Node>();
}

PhxEngine::Renderer::Scene::Scene(
	std::shared_ptr<Core::IFileSystem> fileSystem,
	std::shared_ptr<TextureCache> textureCache,
	RHI::IGraphicsDevice* graphicsDevice)
	: m_fs(fileSystem)
	, m_textureCache(textureCache)
	, m_graphicsDevice(graphicsDevice)
{
}
