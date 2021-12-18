#pragma once

#include <PhxEngine/App/Application.h>
#include <PhxEngine/RHI/PhxRHI.h>

class TestBedApp : public PhxEngine::ApplicationBase
{
public:
    void LoadContent() override;
    void RenderScene() override;

private:
    PhxEngine::RHI::TextureHandle m_depthBuffer;
    PhxEngine::RHI::GraphicsPSOHandle m_geomtryPassPso;
};