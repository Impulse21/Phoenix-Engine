#pragma once

#include <PhxEngine/Engine/Layer.h>
#include <PhxEngine/Engine/EngineApp.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Scene/Scene.h>
#include <DirectXMath.h>
#include <PhxEngine/Renderer/Renderer.h>


#include "EditorCameraController.h"

class SceneRenderLayer : public PhxEngine::AppLayer
{
public:
	SceneRenderLayer();

	void OnAttach() override;
	void OnDetach() override;

	void OnUpdate(PhxEngine::Core::TimeStep const& dt) override;
	void OnRender() override;

	PhxEngine::RHI::TextureHandle& GetFinalColourBuffer()
	{
		return PhxEngine::Renderer::IRenderer::Ptr->GetFinalColourBuffer();
	}
	void SetScene(std::shared_ptr<PhxEngine::Scene::Scene> scene) { this->m_scene = std::move(scene); }
	void ResizeSurface(DirectX::XMFLOAT2 const& size);

private:
	std::shared_ptr<PhxEngine::Scene::Scene> m_scene;
	PhxEngine::Scene::CameraComponent m_editorCamera;
	EditorCameraController m_editorCameraController;
};

