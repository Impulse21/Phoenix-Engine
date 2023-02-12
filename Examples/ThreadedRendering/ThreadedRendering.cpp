#include <PhxEngine/PhxEngine.h>
 
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Core/Helpers.h>
#include <PhxEngine/Core/Platform.h>
#include <PhxEngine/Core/Span.h>

using namespace PhxEngine;

class ThreadedRendering : public EngineRenderPass
{
private:

public:
    ThreadedRendering(IPhxEngineRoot* root)
        : EngineRenderPass(root)
    {
    }

    bool Initialize()
    {
        std::filesystem::path appShaderPath = Core::Platform::GetExcecutableDir() / "shaders/ThreadedRendering/dxil";
        std::vector<uint8_t> shaderByteCode;
        {
            std::filesystem::path shaderFile = appShaderPath / "ColourVS.cso";
            PhxEngine::Core::Helpers::FileRead(shaderFile.generic_string(), shaderByteCode);

            Core::Span span(shaderByteCode);
            this->m_vertexShader = this->GetGfxDevice()->CreateShader(
                {
                    .Stage = RHI::ShaderStage::Vertex,
                    .DebugName = "ColourVS",
                },
                Core::Span(shaderByteCode));
        }

        shaderByteCode.clear();
        {
            std::filesystem::path shaderFile = appShaderPath / "ColourPS.cso";
            PhxEngine::Core::Helpers::FileRead(shaderFile.generic_string(), shaderByteCode);

            Core::Span span(shaderByteCode);
            this->m_pixelShader = this->GetGfxDevice()->CreateShader(
                {
                    .Stage = RHI::ShaderStage::Pixel,
                    .DebugName = "ColourPS",
                },
                Core::Span(shaderByteCode));
        }

        return true;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->GetGfxDevice()->DeleteGraphicsPipeline(this->m_pipeline);
        this->m_pipeline = {};
    }

    void Render() override
    {
        if (!this->m_pipeline.IsValid())
        {
            
            this->m_pipeline = this->GetGfxDevice()->CreateGraphicsPipeline(
                {
                    .VertexShader = this->m_vertexShader,
                    .PixelShader = this->m_pixelShader,
                    .DepthStencilRenderState = {
                        .DepthTestEnable = false
                    },
                    .RtvFormats = { this->GetGfxDevice()->GetTextureDesc(this->GetGfxDevice()->GetBackBuffer()).Format },
                });
        }

        RHI::ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();
        {
            auto _ = commandList->BeginScopedMarker("Render Cube");
            commandList->BeginRenderPassBackBuffer();

            commandList->SetGraphicsPipeline(this->m_pipeline);
            auto canvas = this->GetRoot()->GetCanvasSize();
            RHI::Viewport v(canvas.x, canvas.y);
            commandList->SetViewports(&v, 1);

            RHI::Rect rec(LONG_MAX, LONG_MAX);
            commandList->SetScissors(&rec, 1);
            commandList->Draw(3);

            commandList->EndRenderPass();
        }
        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({commandList});
    }

private:
    RHI::ShaderHandle m_vertexShader;
    RHI::ShaderHandle m_pixelShader;
    RHI::GraphicsPipelineHandle m_pipeline;
};

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int __argc, const char** __argv)
#endif
{
    std::unique_ptr<IPhxEngineRoot> root = CreateEngineRoot();

    EngineParam params = {};
    params.Name = "PhxEngine Example: Basic Triangle";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    root->Initialize(params);

    {
        ThreadedRendering example(root.get());
        if (example.Initialize())
        {
            root->AddPassToBack(&example);
            root->Run();
            root->RemovePass(&example);
        }
    }

    root->Finalizing();
    root.reset();
    RHI::ReportLiveObjects();

    return 0;
}
