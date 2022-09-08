#pragma once

#include <PhxEngine/App/Layer.h>
#include <PhxEngine/Scene/Entity.h>

#include <DirectXMath.h>

class SceneRenderLayer;

class SceneExplorerPanel
{
public:
	SceneExplorerPanel() = default;

	void OnRenderImGui();

	void SetScene(std::shared_ptr<PhxEngine::Scene::Scene> scene) { this->m_scene = scene; }

private:
	void DrawEntityNode(PhxEngine::Scene::Entity entity);
	void DrawEntityComponents(PhxEngine::Scene::Entity entity);

private:
	std::shared_ptr<PhxEngine::Scene::Scene> m_scene;
	PhxEngine::Scene::Entity m_selectedEntity;
};

class EditorLayer : public PhxEngine::AppLayer
{
public:
	EditorLayer(std::shared_ptr<SceneRenderLayer> sceneRenderLayer);

	void OnAttach() override;
	void OnDetach() override;
	void OnRenderImGui() override;

private:
	void BeginDockspace();
	void EndDockSpace();

private:
	std::shared_ptr<PhxEngine::Scene::Scene> m_scene;
	DirectX::XMFLOAT2 m_viewportSize;
	std::shared_ptr<SceneRenderLayer> m_sceneRenderLayer;
	SceneExplorerPanel m_sceneExplorerPanel;
};

