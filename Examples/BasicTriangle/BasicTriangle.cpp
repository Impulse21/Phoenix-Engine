#include <PhxEngine/PhxEngine.h>
 
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Core/Helpers.h>
#include <PhxEngine/Core/Platform.h>
#include <PhxEngine/Core/Span.h>

using namespace PhxEngine;

class BasicTriangle : public EngineRenderPass
{
private:

public:
    BasicTriangle(IPhxEngineRoot* root)
        : EngineRenderPass(root)
    {
    }

    bool Initialize()
    {
        std::filesystem::path appShaderPath = Core::Platform::GetExcecutableDir() / "shaders/BasicTriangle/dxil";
        std::vector<uint8_t> shaderByteCode;
        {
            std::filesystem::path shaderFile = appShaderPath / "BasicTriangleVS.cso";
            PhxEngine::Core::Helpers::FileRead(shaderFile.generic_string(), shaderByteCode);

            Core::Span span(shaderByteCode);
            this->m_vertexShader = this->GetRoot()->GetRHI()->CreateShader(
                {
                    .Stage = RHI::ShaderStage::Vertex,
                    .DebugName = "BasicTriangleVS",
                },
                Core::Span(shaderByteCode));
        }

        shaderByteCode.clear();
        {
            std::filesystem::path shaderFile = appShaderPath / "BasicTrianglePS.cso";
            PhxEngine::Core::Helpers::FileRead(shaderFile.generic_string(), shaderByteCode);

            Core::Span span(shaderByteCode);
            this->m_pixelShader = this->GetRoot()->GetRHI()->CreateShader(
                {
                    .Stage = RHI::ShaderStage::Pixel,
                    .DebugName = "BasicTrianglePS",
                },
                Core::Span(shaderByteCode));
        }

        return true;
    }

    void Render(RHI::IRHIFrameRenderCtx* frameRenderer) override
    {
        if (!this->m_pipeline)
        {
            this->m_pipeline = this->GetRoot()->GetRHI()->CreateGraphicsPipeline(
                {
                    .VertexShader = this->m_vertexShader,
                    .PixelShader = this->m_pixelShader,
                    .DepthStencilRenderState = {
                        .DepthTestEnable = false
                    },
                    .RenderTargets = { frameRenderer->GetBackBuffer() },
                });
        }

        RHI::IRHICommandList* commandList = frameRenderer->BeginCommandRecording();

        {
            auto _ = commandList->BeginScopedMarker("Render Triagnle");
            commandList->BeginRenderPassBackBuffer();

            commandList->SetGraphicsPipeline(this->m_pipeline);
            commandList->Draw(3);

            commandList->EndRenderPass();
        }

        frameRenderer->SubmitCommands({ commandList });
    }

private:
    RHI::RHIShaderHandle m_vertexShader;
    RHI::RHIShaderHandle m_pixelShader;
    RHI::RHIGraphicsPipelineHandle m_pipeline;
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
        BasicTriangle example(root.get());
        if (example.Initialize())
        {
            root->AddPassToBack(&example);
            root->Run();
            root->RemovePass(&example);
        }
    }

    root->Finalizing();
    return 0;
}
