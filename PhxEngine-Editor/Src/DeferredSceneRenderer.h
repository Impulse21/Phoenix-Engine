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
        PSO_Sky,
        PSO_EnvCapture_SkyProcedural,

        // -- Post Process ---
        PSO_ToneMappingPass,

        NumPsoTypes
    };

    enum RenderPassType
    {
        RenderPass_GBuffer,
        RenderPass_DeferredLighting,
        RenderPass_Sky,
        RenderPass_PostFx,
        NumRenderPassTypes
    };

public:
    void Initialize() override;
    void Finialize() override
    {
        this->FreeResources();
    }

    void Update(PhxEngine::Scene::Scene& scene);

    void RenderScene(PhxEngine::Scene::CameraComponent const& camera, PhxEngine::Scene::Scene& scene);

    PhxEngine::RHI::TextureHandle& GetFinalColourBuffer() override 
    {
        return this->m_finalColourBuffer;
    }

    void OnWindowResize(DirectX::XMFLOAT2 const& size) override;

private:
    struct GBuffer
    {
        PhxEngine::RHI::TextureHandle AlbedoTexture;
        PhxEngine::RHI::TextureHandle NormalTexture;
        PhxEngine::RHI::TextureHandle SurfaceTexture;

        PhxEngine::RHI::TextureHandle _PostionTexture;
    };

    void FreeResources();
    void FreeTextureResources();

    void PrepareFrameRenderData(
        PhxEngine::RHI::CommandListHandle commandList,
        PhxEngine::Scene::Scene& scene);

    void CreatePSOs();
    void CreateRenderTargets(DirectX::XMFLOAT2 const& size);

    // Potential Render Functions
private:
    void RefreshEnvProbs(PhxEngine::Scene::CameraComponent const& camera, PhxEngine::Scene::Scene& scene, PhxEngine::RHI::CommandListHandle commandList);
    void DrawMeshes(PhxEngine::Scene::Scene& scene, PhxEngine::RHI::CommandListHandle commandList);


    // Scene Update Systems -> Should be coupled with Scene?
private:
    void RunProbeUpdateSystem(PhxEngine::Scene::Scene& scene);

private:
    PhxEngine::RHI::CommandListHandle m_commandList;
    PhxEngine::RHI::CommandListHandle m_computeCommandList;

    // -- Scene CPU Buffers ---
    // 
    // Uploaded every frame....Could be improved upon.
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
    PhxEngine::RHI::TextureHandle m_finalColourBuffer;
    PhxEngine::RHI::TextureHandle m_depthBuffer;

    DirectX::XMFLOAT2 m_canvasSize;

    // -- Scene Env Propes ---
    static constexpr uint32_t kEnvmapCount = 16;
    static constexpr uint32_t kEnvmapRes = 128;
    static constexpr PhxEngine::RHI::FormatType kEnvmapFormat = PhxEngine::RHI::FormatType::R11G11B10_FLOAT;
    static constexpr uint32_t kEnvmapMIPs = 8;
    static constexpr uint32_t kEnvmapMSAASampleCount = 8;

    PhxEngine::RHI::TextureHandle m_envMapArray;

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

    std::array<PhxEngine::RHI::RenderPassHandle, NumRenderPassTypes> m_renderPasses;
};


