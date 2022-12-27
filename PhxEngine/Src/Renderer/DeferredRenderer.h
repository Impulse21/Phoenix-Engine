#pragma once

#include <PhxEngine/Graphics/ShaderStore.h>
#include <PhxEngine/Core/Span.h>
#include <Shaders/ShaderInteropStructures.h>
#include <PhxEngine/Graphics/IRenderer.h>
#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <PhxEngine/Graphics/CascadeShadowMap.h>
#include <array>
#include <vector>
#include <entt.hpp>
#include <memory>

#include "Visibility.h"
#include "DrawQueue.h"

namespace PhxEngine::Renderer
{
    class DeferredRenderer : public IRenderer
    {
        enum InputLayouts
        {
            IL_PosCol,

            NumILTypes
        };

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

            // -- Debug ---
            PSO_Debug_Cube,

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
            RenderPass_Debug,
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

        void CreateInputLayouts();
        void CreatePSOs();
        void CreateRenderTargets(DirectX::XMFLOAT2 const& size);

        void DebugDrawWorld(PhxEngine::Scene::Scene& scene, Scene::CameraComponent const& camera, PhxEngine::RHI::CommandListHandle commandList);

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

        std::array<PhxEngine::RHI::InputLayoutHandle, InputLayouts::NumILTypes> m_inputLayouts;
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

        Renderer::CullResults m_cullResults;

        struct DrawCubeInfo
        {
            size_t VBOffset = 0;
            size_t IBOffset = 0;
        };

        std::vector<DrawCubeInfo> m_drawInfo;

        struct VertexPosColour
        {
            DirectX::XMFLOAT4 Position;
            DirectX::XMFLOAT4 Colour;
        };

        std::vector<VertexPosColour> m_drawCubeVertices;
        const std::array<uint16_t, 24> kDrawCubeIndices
        {
            0, 2, 2, 3, 0, 1, 1, 3,
            4 ,6, 6, 7, 4, 5, 5, 7,
            0, 4, 2, 6, 1, 5, 3, 7
        };
    };

}

