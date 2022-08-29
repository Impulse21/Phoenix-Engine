#pragma once

#include <PhxEngine/App/Layer.h>
#include <PhxEngine/App/Application.h>
#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Scene/Scene.h>
#include <DirectXMath.h>
#include <PhxEngine/Graphics/IRenderer.h>

class SceneRenderLayer : public PhxEngine::AppLayer
{
public:
	SceneRenderLayer();

	void OnAttach() override;
	void OnDetach() override;

	void OnRender() override;

	PhxEngine::RHI::TextureHandle& GetFinalColourBuffer()
	{
		return this->m_sceneRenderer->GetFinalColourBuffer();
	}
	void SetScene(std::unique_ptr<PhxEngine::Scene::New::Scene> scene) { this->m_scene = std::move(scene); }
	void ResizeSurface(DirectX::XMFLOAT2 const& size);

private:
	std::unique_ptr<PhxEngine::Graphics::IRenderer> m_sceneRenderer;
	std::unique_ptr<PhxEngine::Scene::New::Scene> m_scene;
	PhxEngine::Scene::New::CameraComponent m_editorCamera;
};

