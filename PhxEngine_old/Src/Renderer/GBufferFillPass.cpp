#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/GBufferFillPass.h>
#include <PhxEngine/Graphics/ShaderFactory.h>

using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;


namespace RootParameters
{
    enum
    {
        PushConstant = 0,
        FrameCB,
        CameraCB,
    };
}

PhxEngine::Renderer::GBufferFillPass::GBufferFillPass(RHI::IGraphicsDevice* gfxDevice, std::shared_ptr<CommonPasses> commonPasses)
	: m_gfxDevice(gfxDevice)
	, m_commonPasses(commonPasses)
{
}


void GBufferFillPass::Initialize(Graphics::ShaderFactory& factory)
{
    this->m_vertexShader = factory.CreateShader(
        "PhxEngine/GBufferPassVS.hlsl",
        {
            .Stage = RHI::ShaderStage::Vertex,
            .DebugName = "GBufferPassVS",
        });

    this->m_pixelShader = factory.CreateShader(
        "PhxEngine/GBufferPassPS.hlsl",
        {
            .Stage = RHI::ShaderStage::Vertex,
            .DebugName = "GBufferPassPS",
        });
}

void PhxEngine::Renderer::GBufferFillPass::WindowResized()
{
    this->m_gfxDevice->DeleteGraphicsPipeline(this->m_gfxPipeline);
    this->m_gfxPipeline = {};

    this->m_gfxDevice->DeleteRenderPass(this->m_renderPass);
    this->m_renderPass = {};
}

void PhxEngine::Renderer::GBufferFillPass::BeginPass(ICommandList* commandList, BufferHandle frameCB, Shader::Camera cameraData, GBufferRenderTargets const& gbufferRenderTargets)
{
    commandList->BeginRenderPass(gbufferRenderTargets.RenderPass);

    if (!this->m_gfxPipeline.IsValid())
    {
        // TODO: Need to recreate in the event of format changes - this isn't supported right now.
        this->m_gfxPipeline = this->m_gfxDevice->CreateGraphicsPipeline(
            {
                .VertexShader = this->m_vertexShader,
                .PixelShader = this->m_pixelShader,
                .RtvFormats = {
                    IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.AlbedoTex).Format,
                    IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.NormalTex).Format,
                    IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.SurfaceTex).Format,
                    IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.SpecularTex).Format,
                },
                .DsvFormat = { IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.DepthTex).Format }
            });
    }

    commandList->SetGraphicsPipeline(this->m_gfxPipeline);

    RHI::Viewport v(gbufferRenderTargets.CanvasSize.x, gbufferRenderTargets.CanvasSize.y);
    commandList->SetViewports(&v, 1);

    RHI::Rect rec(LONG_MAX, LONG_MAX);
    commandList->SetScissors(&rec, 1);

    commandList->BindConstantBuffer(RootParameters::FrameCB, frameCB);

    commandList->BindDynamicConstantBuffer(RootParameters::CameraCB, cameraData);
}

void PhxEngine::Renderer::GBufferFillPass::BindPushConstant(RHI::ICommandList* commandList, Shader::GeometryPassPushConstants const& pushData)
{
    commandList->BindPushConstant(RootParameters::PushConstant, pushData);
}
