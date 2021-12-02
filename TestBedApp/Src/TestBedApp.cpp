#include <PhxEngine/Core/Log.h>

#include <PhxEngine/App/Application.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/RHI/PhxRHI_Dx12.h>

#include <string>

using namespace PhxEngine;
using namespace PhxEngine::RHI;

class TestBedApp : public ApplicationBase
{
public:
    void LoadContent() override;
    void RenderScene() override;

private:
    RHI::TextureHandle m_depthBuffer;
};

int main()
{
    PhxEngine::LogSystem::Initialize();

    // Creating Device
    LOG_CORE_INFO("Creating DX12 Graphics Device");
    auto graphicsDevice = std::unique_ptr<IGraphicsDevice>(Dx12::Factory::CreateDevice());

    {
        auto app = std::make_unique<TestBedApp>();
        app->Initialize(graphicsDevice.get());
        app->Run();
        app->Shutdown();
    }

    LOG_CORE_INFO("Shutting down");
    return 0;
}

void TestBedApp::LoadContent()
{
    // -- Create Depth Buffer ---
    {
        TextureDesc desc = {};
        desc.Format = EFormat::D32;
        desc.Width = WindowWidth;
        desc.Height = WindowHeight;
        desc.Dimension = TextureDimension::Texture2D;
        desc.DebugName = "Depth Buffer";
        Color clearValue = { 1.0f, 0.0f, 0.0f, 0.0f };
        desc.OptmizedClearValue = std::make_optional<Color>(clearValue);

        this->m_depthBuffer = this->GetGraphicsDevice()->CreateDepthStencil(desc);
    }
}

void TestBedApp::RenderScene()
{
    auto* backBuffer = this->GetGraphicsDevice()->GetBackBuffer();
    this->GetCommandList()->Open();
    {
        this->GetCommandList()->BeginScropedMarker("Clear Render Target");
        this->GetCommandList()->TransitionBarrier(backBuffer, ResourceStates::Present, ResourceStates::RenderTarget);
        this->GetCommandList()->ClearTextureFloat(backBuffer, { 0.0f, 0.0f, 0.0f, 1.0f });
        this->GetCommandList()->ClearDepthStencilTexture(this->m_depthBuffer, true, 1.0f, false, 0);
    }

    {
        this->GetCommandList()->TransitionBarrier(backBuffer, ResourceStates::RenderTarget, ResourceStates::Present);
    }

    this->GetCommandList()->Close();
    this->GetGraphicsDevice()->ExecuteCommandLists(this->GetCommandList());
}
