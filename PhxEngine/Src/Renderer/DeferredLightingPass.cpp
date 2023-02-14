#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/DeferredLightingPass.h>

#include <PhxEngine/Graphics/ShaderFactory.h>

using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;

DeferredLightingPass::DeferredLightingPass(
    IGraphicsDevice* gfxDevice,
    std::shared_ptr<Renderer::CommonPasses> commonPasses)
    : m_gfxDevice(gfxDevice)
    , m_commonPasses(commonPasses)
{
}


void DeferredLightingPass::Initialize(Graphics::ShaderFactory& factory)
{
    this->m_pixelShader = factory.CreateShader(
        "PhxEngine/DeferredLightingPS.hlsl",
        {
            .Stage = RHI::ShaderStage::Pixel,
            .DebugName = "DeferredLightingPS",
        });

    this->m_computeShader = factory.CreateShader(
        "PhxEngine/DeferredLightingCS.hlsl",
        {
            .Stage = RHI::ShaderStage::Pixel,
            .DebugName = "DeferredLightingCS",
        });
}

void DeferredLightingPass::Render(ICommandList* commandList, Input const& input)
{
    // Set Push Constants for input texture Data.
    if (!this->m_computePso.IsValid())
    {
        this->m_computePso = this->m_gfxDevice->CreateComputePipeline({
                .ComputeShader = this->m_computeShader
            });
    }

    auto _ = commandList->BeginScopedMarker("Deferred Lighting Pass CS");

    RHI::GpuBarrier preBarriers[] =
    {
        RHI::GpuBarrier::CreateTexture(input.Depth, this->m_gfxDevice->GetTextureDesc(input.Depth).InitialState, RHI::ResourceStates::ShaderResource),
        RHI::GpuBarrier::CreateTexture(input.OutputTexture, this->m_gfxDevice->GetTextureDesc(input.OutputTexture).InitialState, RHI::ResourceStates::UnorderedAccess),
    };

    commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preBarriers, _countof(preBarriers)));

    // TODO: Change to match same syntax as GFX pipeline
    commandList->SetComputeState(this->m_computePso);

    commandList->BindConstantBuffer(1, input.FrameBuffer);
    commandList->BindDynamicConstantBuffer(2, *input.CameraData);
    commandList->BindDynamicDescriptorTable(
        4,
        {
            input.Depth,
            input.GBufferAlbedo,
            input.GBufferNormals,
            input.GBufferSurface,
            input.GBufferSpecular
        });
    commandList->BindDynamicUavDescriptorTable(5, { input.OutputTexture });

    auto& outputDesc = this->m_gfxDevice->GetTextureDesc(input.OutputTexture);

    Shader::DefferedLightingCSConstants push = {};
    push.DipatchGridDim =
    {
        outputDesc.Width / DEFERRED_BLOCK_SIZE_X,
        outputDesc.Height / DEFERRED_BLOCK_SIZE_Y,
    };
    push.MaxTileWidth = 16;

    commandList->Dispatch(
        push.DipatchGridDim.x,
        push.DipatchGridDim.y,
        1);

    RHI::GpuBarrier postTransition[] =
    {
        RHI::GpuBarrier::CreateTexture(input.Depth, RHI::ResourceStates::ShaderResource, this->m_gfxDevice->GetTextureDesc(input.Depth).InitialState),
        RHI::GpuBarrier::CreateTexture(input.OutputTexture, RHI::ResourceStates::UnorderedAccess, this->m_gfxDevice->GetTextureDesc(input.OutputTexture).InitialState),
    };

    commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postTransition, _countof(postTransition)));
}