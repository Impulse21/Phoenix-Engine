#include "SceneRenderLayer.h"
#include <PhxEngine/Graphics/ShaderStore.h>
#include <PhxEngine/Core/Span.h>
#include <Shaders/ShaderInteropStructures.h>
#include "DeferredSceneRenderer.h"

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;

using namespace PhxEngine::RHI;

#define USE_DUMMY_RENDERER 1

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
        void RenderScene(PhxEngine::Scene::New::CameraComponent const& camera, PhxEngine::Scene::New::Scene const& scene) {}

        PhxEngine::RHI::TextureHandle GetFinalColourBuffer() override { return TextureHandle(); }

        void OnWindowResize(DirectX::XMFLOAT2 const& size) override {};
    };
}

// ----------------------------------------------------------------------------

SceneRenderLayer::SceneRenderLayer()
	: AppLayer("Scene Render Layer")
{
#ifdef USE_DUMMY_RENDERER
    this->m_sceneRenderer = std::make_unique<DummyRenderer>();
#else
    this->m_sceneRenderer = std::make_unique<DeferredRenderer>();
#endif
}

void SceneRenderLayer::OnAttach()
{
    this->m_sceneRenderer->Initialize();
}

void SceneRenderLayer::OnDetach()
{
    this->m_sceneRenderer->Finialize();
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
    this->m_sceneRenderer->OnWindowResize(size);
}
