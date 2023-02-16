#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/VisibilityBufferPasses.h>
#include <PhxEngine/Graphics/ShaderFactory.h>


using namespace PhxEngine::Renderer;

PhxEngine::Renderer::VisibilityBufferFillPass::VisibilityBufferFillPass(
	RHI::IGraphicsDevice* gfxDevice,
	std::shared_ptr<CommonPasses> commonPasses)
	: m_gfxDevice(gfxDevice)
	, m_commonPasses(commonPasses)
{
}

void PhxEngine::Renderer::VisibilityBufferFillPass::Initialize(Graphics::ShaderFactory& factory)
{
    this->m_vertexShader = factory.CreateShader(
        "PhxEngine/VisibilityBufferFillPassVS.hlsl",
        {
            .Stage = RHI::ShaderStage::Vertex,
            .DebugName = "VisibilityBufferFillPassVS",
        });

    this->m_pixelShader = factory.CreateShader(
        "PhxEngine/VisibilityBufferFillPassPS.hlsl",
        {
            .Stage = RHI::ShaderStage::Pixel,
            .DebugName = "VisibilityBufferFillPassPS",
        });
}

void PhxEngine::Renderer::VisibilityBufferFillPass::WindowResized()
{
    this->m_gfxPipelines = {};
}

void PhxEngine::Renderer::VisibilityBufferFillPass::BeginPass(RHI::ICommandList* commandList, RHI::BufferHandle frameCB, Shader::Camera cameraData, RHI::RenderPassHandle renderPass)
{
    commandList->BeginRenderPass(renderPass);

    if (!this->m_gfxPipelines[ePipelineOpaque].IsValid())
    {
        // TODO: Need to recreate in the event of format changes - this isn't supported right now.
        /*
        this->m_gfxPipelines[ePipelineOpaque] = this->m_gfxDevice->CreateGraphicsPipeline(
            {
                .VertexShader = this->m_vertexShader,
                .PixelShader = this->m_pixelShader,
                .RtvFormats = {
                    IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.AlbedoTex).Format,
                },
                .DsvFormat = { IGraphicsDevice::GPtr->GetTextureDesc(gbufferRenderTargets.DepthTex).Format }
            });
            */
    }

    commandList->SetGraphicsPipeline(this->m_gfxPipelines[ePipelineOpaque]);

    /*
    RHI::Viewport v(gbufferRenderTargets.CanvasSize.x, gbufferRenderTargets.CanvasSize.y);
    commandList->SetViewports(&v, 1);

    RHI::Rect rec(LONG_MAX, LONG_MAX);
    commandList->SetScissors(&rec, 1);

    commandList->BindConstantBuffer(RootParameters::FrameCB, frameCB);

    commandList->BindDynamicConstantBuffer(RootParameters::CameraCB, cameraData);
    */
}

void PhxEngine::Renderer::VisibilityBufferFillPass::BindPushConstant(RHI::ICommandList* commandList, Shader::GeometryPassPushConstants const& pushData)
{
    commandList->BindPushConstant(0, pushData);
}

PhxEngine::Renderer::MeshletBufferFillPass::MeshletBufferFillPass(
    RHI::IGraphicsDevice* gfxDevice,
    std::shared_ptr<CommonPasses> commonPasses)
    : m_gfxDevice(gfxDevice)
    , m_commonPasses(commonPasses)
{
}

void PhxEngine::Renderer::MeshletBufferFillPass::Initialize(Graphics::ShaderFactory& factory)
{
    this->m_computeShader = factory.CreateShader(
        "PhxEngine/MeshletBufferFillPassCS.hlsl.hlsl",
        {
            .Stage = RHI::ShaderStage::Compute,
            .DebugName = "MeshletBufferFillPassCS",
        });
}

void PhxEngine::Renderer::MeshletBufferFillPass::Dispatch(RHI::ICommandList* commandList, RHI::BufferHandle meshletBuffer, size_t numInstances)
{
    if (!this->m_computePipeline.IsValid())
    {
        this->m_computePipeline = this->m_gfxDevice->CreateComputePipeline({
            .ComputeShader = this->m_computeShader,
            });
    }

    auto _ = commandList->BeginScopedMarker("Meshlet Buffer Fill CS");

    RHI::GpuBarrier preBarriers[] =
    {
        RHI::GpuBarrier::CreateBuffer(meshletBuffer, this->m_gfxDevice->GetBufferDesc(meshletBuffer).InitialState, RHI::ResourceStates::UnorderedAccess),
    };
    commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preBarriers, _countof(preBarriers)));

    commandList->BindDynamicUavDescriptorTable(0, { meshletBuffer }, {});
    commandList->Dispatch(numInstances);

    RHI::GpuBarrier postBarriers[] =
    {
        RHI::GpuBarrier::CreateBuffer(meshletBuffer, RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetBufferDesc(meshletBuffer).InitialState),
    };

    commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postBarriers, _countof(postBarriers)));

}
