#pragma once

#include <memory>

#include <PhxEngine/App/Application.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Renderer/TextureCache.h>
#include <PhxEngine/Renderer/Scene.h>

#include <DirectXMath.h>

class PbrDemo : public PhxEngine::ApplicationBase
{
protected:
    void LoadContent() override;
    void RenderScene() override;
    void Update(double elapsedTime) override;

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

    DirectX::XMMATRIX m_viewMatrix = DirectX::XMMatrixIdentity();

    PhxEngine::RHI::TextureHandle m_skyboxTexture;
    PhxEngine::RHI::TextureHandle m_irradanceMap;
    PhxEngine::RHI::TextureHandle m_prefilteredMap;
    PhxEngine::RHI::TextureHandle m_brdfLUT;

    // Camera pitch
    float m_pitch = 0.0f;
    float m_yaw = 0.0f;

    DirectX::XMFLOAT3 m_sunDirection = { 0.0f, 0.0f, 1.0f };
};