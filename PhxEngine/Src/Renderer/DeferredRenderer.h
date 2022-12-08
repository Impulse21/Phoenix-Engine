#pragma once

#include <PhxEngine/Graphics/ShaderStore.h>
#include <PhxEngine/Core/Span.h>
#include <Shaders/ShaderInteropStructures.h>
#include <PhxEngine/Graphics/IRenderer.h>
#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <PhxEngine/Graphics/CascadeShadowMap.h>
#include "DrawQueue.h"
#include <array>
#include <vector>
#include <entt.hpp>
#include <memory>

namespace PhxEngine::Renderer
{
    class DeferredRenderer : public IRenderer
    {
        enum PsoType
        {
            PSO_GBufferPass = 0,
            PSO_FullScreenQuad,
            PSO_DeferredLightingPass,
            PSO_DeferredLightingPass_RTShadows,
            PSO_Sky,
            PSO_EnvCapture_SkyProcedural,
            PSO_Shadow,

            // -- Post Process ---
            PSO_ToneMappingPass,

            NumPsoTypes
        };

        enum PsoComputeType
        {
            PSO_GenerateMipMaps_TextureCubeArray = 0,
            PSO_FilterEnvMap,

            NumComputePsoTypes
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

        void RenderScene(PhxEngine::Scene::CameraComponent const& camera, PhxEngine::Scene::Scene& scene);

        PhxEngine::RHI::TextureHandle& GetFinalColourBuffer() override
        {
            return this->m_finalColourBuffer;
        }

        void OnWindowResize(DirectX::XMFLOAT2 const& size) override;

        void OnReloadShaders() override {};

    private:
        struct GBuffer
        {
            PhxEngine::RHI::TextureHandle AlbedoTexture;
            PhxEngine::RHI::TextureHandle NormalTexture;
            PhxEngine::RHI::TextureHandle SurfaceTexture;
            PhxEngine::RHI::TextureHandle SpecularTexture;
        };

        void FreeResources();
        void FreeTextureResources();

        void PrepareFrameRenderData(
            PhxEngine::RHI::CommandListHandle commandList,
            PhxEngine::Scene::CameraComponent const& mainCamera,
            PhxEngine::Scene::Scene& scene);

        void CreatePSOs();
        void CreateRenderTargets(DirectX::XMFLOAT2 const& size);

        // Potential Render Functions
    private:
        void UpdateRaytracingAccelerationStructures(PhxEngine::Scene::Scene& scene, PhxEngine::RHI::CommandListHandle commandList);
        void RefreshEnvProbes(PhxEngine::Scene::CameraComponent const& camera, PhxEngine::Scene::Scene& scene, PhxEngine::RHI::CommandListHandle commandList);

        void DrawMeshes(
            DrawQueue const& drawQueue,
            PhxEngine::Scene::Scene& scene,
            PhxEngine::RHI::CommandListHandle commandList,
            const RenderCam* renderCams = nullptr,
            uint32_t numRenderCameras = 1);

    private:
        PhxEngine::RHI::CommandListHandle m_commandList;
        PhxEngine::RHI::CommandListHandle m_computeCommandList;

        std::array<PhxEngine::RHI::GraphicsPSOHandle, PsoType::NumPsoTypes> m_pso;
        std::array<PhxEngine::RHI::ComputePSOHandle, PsoComputeType::NumComputePsoTypes> m_psoCompute;

        // -- Textures ---
        GBuffer m_gBuffer;
        PhxEngine::RHI::TextureHandle m_deferredLightBuffer;
        PhxEngine::RHI::TextureHandle m_finalColourBuffer;
        PhxEngine::RHI::TextureHandle m_depthBuffer;

        DirectX::XMFLOAT2 m_canvasSize;

        static constexpr uint32_t kCascadeShadowMapRes = 1024;
        static constexpr PhxEngine::RHI::FormatType kCascadeShadowMapFormat = PhxEngine::RHI::FormatType::D16;

        enum ConstantBufferTypes
        {
            CB_Frame = 0,
            NumCB
        };
        std::array<PhxEngine::RHI::BufferHandle, NumCB> m_constantBuffers;

        std::array<PhxEngine::RHI::RenderPassHandle, NumRenderPassTypes> m_renderPasses;
        std::unique_ptr<PhxEngine::Graphics::CascadeShadowMap> m_cascadeShadowMaps;
    };

}

