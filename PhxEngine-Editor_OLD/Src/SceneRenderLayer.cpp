#include "SceneRenderLayer.h"
#include <PhxEngine/Graphics/ShaderStore.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Shaders/ShaderInteropStructures.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;

#define USE_DUMMY_RENDERER 0

// ----------------------------------------------------------------------------

SceneRenderLayer::SceneRenderLayer()
	: AppLayer("Scene Render Layer")
{
}

void SceneRenderLayer::OnAttach()
{
    // DirectX::XMVECTOR eyePos =  DirectX::XMVectorSet(0.0f, 2.0f, 4.0f, 1.0f);
    DirectX::XMVECTOR eyePos =  DirectX::XMVectorSet(0.0f, 2.0f, 10.0f, 1.0f);
    DirectX::XMVECTOR focusPoint = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    DirectX::XMVECTOR eyeDir = DirectX::XMVectorSubtract(focusPoint, eyePos);
    eyeDir = DirectX::XMVector3Normalize(eyeDir);

    DirectX::XMStoreFloat3(
        &this->m_editorCamera.Eye,
        eyePos);

    DirectX::XMStoreFloat3(
        &this->m_editorCamera.Forward,
        eyeDir);

    this->m_editorCamera.Width = EngineApp::Ptr->GetSpec().WindowWidth;
    this->m_editorCamera.Height = EngineApp::Ptr->GetSpec().WindowHeight;
    this->m_editorCamera.FoV = 1.7;
    this->m_editorCamera.UpdateCamera();
}

void SceneRenderLayer::OnDetach()
{
}

void SceneRenderLayer::OnUpdate(PhxEngine::Core::TimeStep const& dt)
{
    this->m_editorCameraController.OnUpdate(dt, this->m_editorCamera);
    if (this->m_scene)
    {
        this->m_scene->OnUpdate();
    }
}

void SceneRenderLayer::OnRender()
{
    if (this->m_scene)
    {
        IRenderer::Ptr->RenderScene(this->m_editorCamera, *this->m_scene);
    }
}

void SceneRenderLayer::ResizeSurface(DirectX::XMFLOAT2 const& size)
{
    this->m_editorCamera.Width = size.x;
    this->m_editorCamera.Height = size.y;
    this->m_editorCamera.UpdateCamera();
    IRenderer::Ptr->OnWindowResize(size);
}
