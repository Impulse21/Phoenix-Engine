#pragma once


#include <PhxEngine/App/Application.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Renderer/TextureCache.h>
#include <PhxEngine/Renderer/Scene.h>

class GltfViewer : public PhxEngine::ApplicationBase
{
protected:
    void LoadContent() override;
    void RenderScene() override;
    void Update(double elapsedTime) override;

private:
    void WriteSeceneData();
    void CreatePipelineStateObjects();

    void CreateRenderTargets();

private:
    std::shared_ptr<PhxEngine::Core::IFileSystem> m_fs;
    std::shared_ptr<PhxEngine::Renderer::TextureCache> m_textureCache;


    PhxEngine::RHI::GraphicsPSOHandle m_geomtryPassPso;
    PhxEngine::RHI::TextureHandle m_depthBuffer;

    PhxEngine::Renderer::New::Scene m_scene;
};

