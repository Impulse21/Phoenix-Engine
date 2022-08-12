#pragma once

#include <PhxEngine/App/Layer.h>
#include <DirectXMath.h>

class SceneRenderLayer;

class EditorLayer : public PhxEngine::AppLayer
{
public:
	EditorLayer(std::shared_ptr<SceneRenderLayer> sceneRenderLayer)
		: AppLayer("Editor Layer")
		, m_sceneRenderLayer(sceneRenderLayer)
	{};

	void OnRenderImGui() override;
	

private:
	void BeginDockspace();
	void EndDockSpace();

private:
	DirectX::XMFLOAT2 m_viewportSize;
	std::shared_ptr<SceneRenderLayer> m_sceneRenderLayer;
};

