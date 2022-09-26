#include "SceneRenderLayer.h"
#include <PhxEngine/Graphics/ShaderStore.h>
#include <PhxEngine/Core/Span.h>
#include <Shaders/ShaderInteropStructures.h>
#include "DeferredSceneRenderer.h"

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;

using namespace PhxEngine::RHI;

#define USE_DUMMY_RENDERER 0

namespace
{

    class DummyRenderer : public PhxEngine::Graphics::IRenderer
    {
        enum PsoType
        {
            PSO_GBufferPass = 0,
            PSO_FullScreenQuad,
            PSO_DeferredLightingPass,

            NumPsoTypes
        };

    public:
        void Initialize() override {}
        void Finialize() override {}
        void RenderScene(PhxEngine::Scene::CameraComponent const& camera, PhxEngine::Scene::Scene& scene) {}

        PhxEngine::RHI::TextureHandle& GetFinalColourBuffer() override { return m_dummyHandle; }

        void OnWindowResize(DirectX::XMFLOAT2 const& size) override {};

    private:
        TextureHandle m_dummyHandle = {};
    };
}

// ----------------------------------------------------------------------------

SceneRenderLayer::SceneRenderLayer()
	: AppLayer("Scene Render Layer")
{
#if USE_DUMMY_RENDERER
    this->m_sceneRenderer = std::make_unique<DummyRenderer>();
#else
    this->m_sceneRenderer = std::make_unique<DeferredRenderer>();
#endif
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

    this->m_editorCamera.Width = LayeredApplication::Ptr->GetSpec().WindowWidth;
    this->m_editorCamera.Height = LayeredApplication::Ptr->GetSpec().WindowHeight;
    this->m_editorCamera.FoV = 1.7;
    this->m_editorCamera.ZNear = 0.1;
    this->m_editorCamera.ZFar = 10000;
    this->m_editorCamera.UpdateCamera();

    this->m_sceneRenderer->Initialize();
}

void SceneRenderLayer::OnDetach()
{
    this->m_sceneRenderer->Finialize();
}

void SceneRenderLayer::OnUpdate(PhxEngine::Core::TimeStep const& dt)
{
    this->m_editorCameraController.OnUpdate(dt, this->m_editorCamera);
    if (this->m_scene)
    {
        this->m_sceneRenderer->Update(*this->m_scene);
    }
}

void SceneRenderLayer::OnRender()
{
    if (this->m_scene)
    {
        this->m_sceneRenderer->RenderScene(this->m_editorCamera, *this->m_scene);
    }
}

void SceneRenderLayer::ResizeSurface(DirectX::XMFLOAT2 const& size)
{
    this->m_editorCamera.Width = size.x;
    this->m_editorCamera.Height = size.y;
    this->m_editorCamera.UpdateCamera();
    this->m_sceneRenderer->OnWindowResize(size);
}
