#pragma once

#include <memory>

#include <PhxEngine/App/Application.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Renderer/TextureCache.h>
#include <PhxEngine/Renderer/Scene.h>

class PbrDemo : public PhxEngine::ApplicationBase
{
public:
    void LoadContent() override;
    void RenderScene() override;

private:
    void CreateRenderTargets();

    void CreatePipelineStates();

    void CreateScene();

private:
    PhxEngine::RHI::TextureHandle m_depthBuffer;
    PhxEngine::RHI::GraphicsPSOHandle m_geomtryPassPso;
    std::shared_ptr<PhxEngine::Core::IFileSystem> m_fs;
    std::shared_ptr<PhxEngine::Renderer::TextureCache> m_textureCache;
    std::unique_ptr<PhxEngine::Renderer::Scene> m_scene;
};