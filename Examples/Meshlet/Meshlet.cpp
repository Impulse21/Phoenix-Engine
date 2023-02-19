#include <PhxEngine/PhxEngine.h>
 
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Core/Helpers.h>
#include <PhxEngine/Core/Platform.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Entity.h>
#include <PhxEngine/Scene/SceneLoader.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Graphics/TextureCache.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Renderer/CommonPasses.h>
#include <PhxEngine/Renderer/VisibilityBufferPasses.h>
#include <PhxEngine/Renderer/DeferredLightingPass.h>
#include <PhxEngine/Renderer/ToneMappingPass.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Engine/ApplicationBase.h>
#include <PhxEngine/Renderer/GBuffer.h>
#include <PhxEngine/Engine/CameraControllers.h>

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;

class VisibilityRenderering : public EngineRenderPass
{
private:

public:
    VisibilityRenderering(IPhxEngineRoot* root)
        : EngineRenderPass(root)
    {
    }

    bool Initialize()
    {
        const std::filesystem::path appShaderPath = Core::Platform::GetExcecutableDir() / "Shaders/Meshlet/dxil";

        std::shared_ptr<Core::IFileSystem> nativeFS = Core::CreateNativeFileSystem();
        this->m_shaderFactory = std::make_unique<Graphics::ShaderFactory>(this->GetGfxDevice(), nativeFS, appShaderPath);

        this->m_amplificationShader = this->m_shaderFactory->CreateShader(
            "BasicTriangleAS.hlsl",
            {
                .Stage = RHI::ShaderStage::Amplification,
                .DebugName = "BasicTriangleAS",
            });

        this->m_meshShader = this->m_shaderFactory->CreateShader(
            "BasicTriangleMS.hlsl",
            {
                .Stage = RHI::ShaderStage::Mesh,
                .DebugName = "BasicTriangleMS",
            });

        this->m_pixelShader = this->m_shaderFactory->CreateShader(
            "BasicTrianglePS.hlsl",
            {
                .Stage = RHI::ShaderStage::Pixel,
                .DebugName = "BasicTrianglePS",
            });

        return true;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->GetGfxDevice()->DeleteMeshPipeline(this->m_pipeline);
        this->m_pipeline = {};
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        this->GetRoot()->SetInformativeWindowTitle("Meshlet", {});
    }

    void Render() override
    {
        if (!this->m_pipeline.IsValid())
        {
            this->m_pipeline = this->GetGfxDevice()->CreateMeshPipeline(
                {
                    .AmpShader = this->m_amplificationShader,
                    .MeshShader = this->m_meshShader,
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

            commandList->SetMeshPipeline(this->m_pipeline);
            auto canvas = this->GetRoot()->GetCanvasSize();
            RHI::Viewport v(canvas.x, canvas.y);
            commandList->SetViewports(&v, 1);

            RHI::Rect rec(LONG_MAX, LONG_MAX);
            commandList->SetScissors(&rec, 1);
            commandList->DispatchMesh(1);

            commandList->EndRenderPass();
        }

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList });
    }


private:
    RHI::ShaderHandle m_amplificationShader;
    RHI::ShaderHandle m_meshShader;
    RHI::ShaderHandle m_pixelShader;
    std::unique_ptr<Graphics::ShaderFactory> m_shaderFactory;
    RHI::MeshPipelineHandle m_pipeline;
};

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int __argc, const char** __argv)
#endif
{
    std::unique_ptr<IPhxEngineRoot> root = CreateEngineRoot();

    EngineParam params = {};
    params.Name = "PhxEngine Example: Shadows";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    params.WindowWidth = 2000;
    params.WindowHeight = 1200;
    root->Initialize(params);

    {
        VisibilityRenderering example(root.get());
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
