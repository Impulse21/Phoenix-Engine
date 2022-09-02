#pragma once

#include <PhxEngine/Graphics/ShaderStore.h>
#include <PhxEngine/Core/Span.h>
#include <Shaders/ShaderInteropStructures.h>
#include <PhxEngine/Graphics/IRenderer.h>
#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <array>
#include <vector>


class DeferredRenderer : public PhxEngine::Graphics::IRenderer
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

    void RenderScene(PhxEngine::Scene::New::CameraComponent const& camera, PhxEngine::Scene::New::Scene& scene);

    PhxEngine::RHI::TextureHandle GetFinalColourBuffer() override { return this->m_deferredLightBuffer; }

    void OnWindowResize(DirectX::XMFLOAT2 const& size) override;

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
        PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_depthBuffer);
        PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_gBuffer.AlbedoTexture);
        PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_gBuffer.NormalTexture);
        PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_gBuffer.SurfaceTexture);
        PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_gBuffer._PostionTexture);
        PhxEngine::RHI::IGraphicsDevice::Ptr->DeleteTexture(this->m_deferredLightBuffer);
    }

    void PrepareFrameRenderData(
        PhxEngine::RHI::CommandListHandle commandList,
        PhxEngine::Scene::New::Scene& scene,
        Shader::Frame& outFrameShaderData);

    void CreatePSOs();
    void CreateRenderTargets(DirectX::XMFLOAT2 const& size);

    void DrawMeshes(PhxEngine::Scene::New::Scene& scene, PhxEngine::RHI::CommandListHandle commandList);

private:
    PhxEngine::RHI::CommandListHandle m_commandList;
    PhxEngine::RHI::CommandListHandle m_computeCommandList;

    // -- Scene CPU Buffers ---
    // 
    // Uploaded every frame....Could be improved upon.
    std::vector<Shader::ShaderLight> m_lightCpuData;
    std::vector<Shader::ShaderLight> m_shadowLights;
    std::vector<DirectX::XMFLOAT4X4> m_matricesCPUData;

    std::array<PhxEngine::RHI::GraphicsPSOHandle, PsoType::NumPsoTypes> m_pso;

    // -- Scene GPU Buffers ---
    size_t m_numGeometryEntires = 0;
    PhxEngine::RHI::BufferHandle m_geometryGpuBuffer;
    std::vector<PhxEngine::RHI::BufferHandle> m_geometryUploadBuffers;

    size_t m_numMaterialEntries = 0;
    PhxEngine::RHI::BufferHandle m_materialGpuBuffer;
    std::vector<PhxEngine::RHI::BufferHandle> m_materialUploadBuffers;

    // -- Textures ---
    GBuffer m_gBuffer;
    PhxEngine::RHI::TextureHandle m_deferredLightBuffer;
    PhxEngine::RHI::TextureHandle m_depthBuffer;

    DirectX::XMFLOAT2 m_canvasSize;


    enum ConstantBufferTypes
    {
        CB_Frame = 0,
        NumCB
    };
    std::array<PhxEngine::RHI::BufferHandle, NumCB> m_constantBuffers;

    enum ResourceBufferTypes
    {
        RB_LightEntities,
        RB_Matrices,
        NumRB
    };
    std::array<PhxEngine::RHI::BufferHandle, NumRB> m_resourceBuffers;
};

