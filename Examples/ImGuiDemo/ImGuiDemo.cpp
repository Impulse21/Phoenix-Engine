#include <PhxEngine/PhxEngine.h>
 
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Core/Helpers.h>
#include <PhxEngine/Core/Platform.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Engine/ImguiRenderer.h>

#include <imgui.h>

using namespace PhxEngine;

class ImGuiDemo : public EngineRenderPass
{
public:
    ImGuiDemo(IPhxEngineRoot* root)
        : EngineRenderPass(root)
    {
    }

    bool Initialize(Graphics::ShaderFactory& factory)
    {
        this->m_vertexShader = factory.CreateShader(
            "ImGuiDemo/BasicTriangleVS.hlsl",
            {
                .Stage = RHI::ShaderStage::Vertex,
                .DebugName = "BasicTriangleVS",
            });

        this->m_pixelShader = factory.CreateShader(
            "ImGuiDemo/BasicTrianglePS.hlsl",
            {
                .Stage = RHI::ShaderStage::Pixel,
                .DebugName = "BasicTrianglePS",
            });

        return true;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->GetGfxDevice()->DeleteGraphicsPipeline(this->m_pipeline);
        this->m_pipeline = {};
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        this->GetRoot()->SetInformativeWindowTitle("Basic Triangle", {});
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
            auto _ = commandList->BeginScopedMarker("Render Triagnle");
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
        this->GetGfxDevice()->ExecuteCommandLists({ commandList });
    }

private:
    RHI::ShaderHandle m_vertexShader;
    RHI::ShaderHandle m_pixelShader;
    RHI::GraphicsPipelineHandle m_pipeline;
};

class ImGuiDemoUI : public ImGuiRenderer
{
private:

public:
    ImGuiDemoUI(IPhxEngineRoot* root)
        : ImGuiRenderer(root)
    {
    }

    void BuildUI() override
    {
        ImGui::ShowDemoWindow();
    }
};

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int __argc, const char** __argv)
#endif
{
    std::unique_ptr<IPhxEngineRoot> root = CreateEngineRoot();

    EngineParam params = {};
    params.Name = "PhxEngine Example: ImGUI";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    root->Initialize(params);

    {

        std::shared_ptr<Core::IFileSystem> nativeFS = Core::CreateNativeFileSystem();

        const std::filesystem::path appShaderPath = Core::Platform::GetExcecutableDir() / "Shaders/ImGuiDemo/dxil";
        std::filesystem::path engineShadersRoot = Core::Platform::GetExcecutableDir() / "Shaders/PhxEngine/dxil";
        std::shared_ptr<Core::IRootFileSystem> rootFilePath = Core::CreateRootFileSystem();
        rootFilePath->Mount("/Shaders/PhxEngine", engineShadersRoot);
        rootFilePath->Mount("/Shaders/ImGuiDemo", appShaderPath);

        auto shaderFactory = std::make_unique<Graphics::ShaderFactory>(root->GetGfxDevice(), rootFilePath, "/Shaders");

        ImGuiDemo example(root.get());
        if (example.Initialize(*shaderFactory))
        {
            root->AddPassToBack(&example);

            ImGuiDemoUI userInterface(root.get());
            if (userInterface.Initialize(*shaderFactory))
            {
                root->AddPassToBack(&userInterface);
            }

            root->Run();
            root->RemovePass(&example);
            root->RemovePass(&userInterface);
        }
    }

    root->Finalizing();
    root.reset();
    RHI::ReportLiveObjects();

    return 0;
}
