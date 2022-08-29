#include "SceneRenderLayer.h"
#include <PhxEngine/Graphics/ShaderStore.h>

using namespace PhxEngine;
using namespace PhxEngine::Scene;

using namespace PhxEngine::RHI;

namespace
{
    class DeferredRenderer : public Graphics::IRenderer
    {
        enum PsoType
        {
            PSO_GBufferPass = 0,
            PSO_FullScreenQuad,
            PSO_DeferredLightingPass,

            NumPsoTypes
        };

    public:
        void Initialize() override;
        void Finialize() override
        {
            this->FreeTextureResources();
        }

        void RenderScene(Scene::New::CameraComponent const& camera, Scene::New::Scene const& scene);

        RHI::TextureHandle GetFinalColourBuffer() override { return this->m_deferredLightBuffer; }

        void OnWindowResize(DirectX::XMFLOAT2 const& size) override
        {
            this->FreeTextureResources();

            this->CreateRenderTargets(size);
        }

    private:
        struct GBuffer
        {
            PhxEngine::RHI::TextureHandle AlbedoTexture;
            PhxEngine::RHI::TextureHandle NormalTexture;
            PhxEngine::RHI::TextureHandle SurfaceTexture;

            PhxEngine::RHI::TextureHandle _PostionTexture;
        };

        void FreeTextureResources()
        {
            RHI::IGraphicsDevice::Ptr->FreeTexture(this->m_depthBuffer);
            RHI::IGraphicsDevice::Ptr->FreeTexture(this->m_gBuffer.AlbedoTexture);
            RHI::IGraphicsDevice::Ptr->FreeTexture(this->m_gBuffer.NormalTexture);
            RHI::IGraphicsDevice::Ptr->FreeTexture(this->m_gBuffer.SurfaceTexture);
            RHI::IGraphicsDevice::Ptr->FreeTexture(this->m_gBuffer._PostionTexture);
            RHI::IGraphicsDevice::Ptr->FreeTexture(this->m_deferredLightBuffer);
        }

        void CreatePSOs();
        void CreateRenderTargets(DirectX::XMFLOAT2 const& size);

    private:
        RHI::CommandListHandle m_commandList;
        RHI::CommandListHandle m_computeCommandList;

        // -- Scene CPU Buffers ---
        std::vector<Shader::Geometry> m_geometryCpuData;
        std::vector<Shader::MaterialData> m_materialCpuData;

        // Uploaded every frame....Could be improved upon.
        std::vector<Shader::ShaderLight> m_lightCpuData;
        std::vector<Shader::ShaderLight> m_shadowLights;
        std::vector<DirectX::XMFLOAT4X4> m_matricesCPUData;

        std::array<GraphicsPSOHandle, PsoType::NumPsoTypes> m_pso;

        GBuffer m_gBuffer;
        PhxEngine::RHI::TextureHandle m_deferredLightBuffer;
        PhxEngine::RHI::TextureHandle m_depthBuffer;
    };


    void DeferredRenderer::Initialize()
    {
        auto& spec = LayeredApplication::Ptr->GetSpec();
        this->CreatePSOs();
        this->CreateRenderTargets({ static_cast<float>(spec.WindowWidth), static_cast<float>(spec.WindowHeight) });

        this->m_commandList = RHI::IGraphicsDevice::Ptr->CreateCommandList();
        RHI::CommandListDesc commandLineDesc = {};
        commandLineDesc.QueueType = RHI::CommandQueueType::Compute;
        this->m_computeCommandList = RHI::IGraphicsDevice::Ptr->CreateCommandList(commandLineDesc);
    }

    void DeferredRenderer::CreateRenderTargets(DirectX::XMFLOAT2 const& size)
    {
        // -- Depth ---
        RHI::TextureDesc desc = {};
        desc.Width = std::max(size.x, 1.0f);
        desc.Height = std::max(size.y, 1.0f);
        desc.Dimension = RHI::TextureDimension::Texture2D;
        desc.IsBindless = false;

        desc.Format = RHI::FormatType::D32;
        desc.IsTypeless = true;
        desc.DebugName = "Depth Buffer";
        desc.OptmizedClearValue.DepthStencil.Depth = 1.0f;
        desc.BindingFlags = desc.BindingFlags | RHI::BindingFlags::DepthStencil;
        desc.InitialState = RHI::ResourceStates::DepthWrite;

        this->m_depthBuffer = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);

        // -- GBuffers ---
        desc.BindingFlags = RHI::BindingFlags::RenderTarget | RHI::BindingFlags::ShaderResource;
        desc.InitialState = RHI::ResourceStates::RenderTarget;

        desc.Format = RHI::FormatType::RGBA32_FLOAT;
        desc.DebugName = "Albedo Buffer";

        this->m_gBuffer.AlbedoTexture = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);

        // desc.Format = RHI::FormatType::R10G10B10A2_UNORM;
        desc.Format = RHI::FormatType::RGBA16_SNORM;
        desc.DebugName = "Normal Buffer";
        this->m_gBuffer.NormalTexture = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);


        desc.Format = RHI::FormatType::RGBA8_UNORM;
        desc.DebugName = "Surface Buffer";
        this->m_gBuffer.SurfaceTexture = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);

        desc.Format = RHI::FormatType::RGBA32_FLOAT;
        desc.DebugName = "Debug Position Buffer";

        // TODO: Determine what format should be used here.
        desc.Format = RHI::FormatType::R10G10B10A2_UNORM;
        desc.DebugName = "Deferred Lighting";
        this->m_deferredLightBuffer = RHI::IGraphicsDevice::Ptr->CreateTexture(desc);
    }

    void DeferredRenderer::CreatePSOs()
    {
        {
            RHI::GraphicsPSODesc psoDesc = {};
            psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_GBufferPass);
            psoDesc.PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_GBufferPass);
            psoDesc.InputLayout = nullptr;

            // psoDesc.RasterRenderState.CullMode = RHI::RasterCullMode::None;
            psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.AlbedoTexture).Format);
            psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.NormalTexture).Format);
            psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer.SurfaceTexture).Format);
            psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(this->m_gBuffer._PostionTexture).Format);
            psoDesc.DsvFormat = IGraphicsDevice::Ptr->GetTextureDesc(this->m_depthBuffer).Format;

            this->m_pso[PSO_GBufferPass] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);
        }

        {
            /*
            RHI::GraphicsPSODesc psoDesc = {};
            psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_ShadowPass);
            psoDesc.InputLayout = nullptr;

            psoDesc.DsvFormat = kShadowAtlasFormat;
            psoDesc.RasterRenderState.DepthBias = 100000;
            psoDesc.RasterRenderState.DepthBiasClamp = 0.0f;
            psoDesc.RasterRenderState.SlopeScaledDepthBias = 1.0f;
            psoDesc.RasterRenderState.DepthClipEnable = false;

            this->m_pso[PSO_] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);
            */
        }

        {
            // TODO: Use the files directly
            RHI::GraphicsPSODesc psoDesc = {};
            psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_FullscreenQuad);
            psoDesc.PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_FullscreenQuad);
            psoDesc.InputLayout = nullptr;
            psoDesc.RasterRenderState.CullMode = RHI::RasterCullMode::Front;
            psoDesc.DepthStencilRenderState.DepthTestEnable = false;
            psoDesc.RtvFormats.push_back(IGraphicsDevice::Ptr->GetTextureDesc(IGraphicsDevice::Ptr->GetBackBuffer()).Format);
            this->m_pso[PSO_FullScreenQuad] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);

            psoDesc.VertexShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::VS_DeferredLighting);
            psoDesc.PixelShader = Graphics::ShaderStore::Ptr->Retrieve(Graphics::PreLoadShaders::PS_DeferredLighting);

            this->m_pso[PSO_DeferredLightingPass] = IGraphicsDevice::Ptr->CreateGraphicsPSO(psoDesc);
        }
    }

    void DeferredRenderer::RenderScene(Scene::New::CameraComponent const& camera, Scene::New::Scene const& scene)
    {

    }
}

// ----------------------------------------------------------------------------

SceneRenderLayer::SceneRenderLayer()
	: AppLayer("Scene Render Layer")
{
    this->m_sceneRenderer = std::make_unique<DeferredRenderer>();
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
