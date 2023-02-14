#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/GBuffer.h>

using namespace PhxEngine::RHI;
using namespace PhxEngine::Renderer;


void GBufferRenderTargets::Initialize(
    RHI::IGraphicsDevice* gfxDevice,
    DirectX::XMFLOAT2 const& size)
{

    CanvasSize = size;

    // -- Depth ---
    RHI::TextureDesc desc = {};
    desc.Width = std::max(size.x, 1.0f);
    desc.Height = std::max(size.y, 1.0f);
    desc.Dimension = RHI::TextureDimension::Texture2D;
    desc.IsBindless = false;

    desc.Format = RHI::RHIFormat::D32;
    desc.IsTypeless = true;
    desc.DebugName = "Depth Buffer";
    desc.OptmizedClearValue.DepthStencil.Depth = 1.0f;
    desc.BindingFlags = RHI::BindingFlags::ShaderResource | RHI::BindingFlags::DepthStencil;
    desc.InitialState = RHI::ResourceStates::DepthWrite;
    this->DepthTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);
    // -- Depth end ---

    // -- GBuffers ---
    desc.OptmizedClearValue.Colour = { 0.0f, 0.0f, 0.0f, 1.0f };
    desc.BindingFlags = RHI::BindingFlags::RenderTarget | RHI::BindingFlags::ShaderResource;
    desc.InitialState = RHI::ResourceStates::ShaderResource;
    desc.IsBindless = true;

    desc.Format = RHI::RHIFormat::RGBA32_FLOAT;
    desc.DebugName = "Albedo Buffer";
    this->AlbedoTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

    // desc.Format = RHI::RHIFormat::R10G10B10A2_UNORM;
    desc.Format = RHI::RHIFormat::RGBA16_SNORM;
    desc.DebugName = "Normal Buffer";
    this->NormalTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

    desc.Format = RHI::RHIFormat::RGBA8_UNORM;
    desc.DebugName = "Surface Buffer";
    this->SurfaceTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

    desc.Format = RHI::RHIFormat::SRGBA8_UNORM;
    desc.DebugName = "Specular Buffer";
    this->SpecularTex = RHI::IGraphicsDevice::GPtr->CreateTexture(desc);

    this->RenderPass = IGraphicsDevice::GPtr->CreateRenderPass(
        {
            .Attachments =
            {
                {
                    .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                    .Texture = this->AlbedoTex,
                    .InitialLayout = RHI::ResourceStates::ShaderResource,
                    .SubpassLayout = RHI::ResourceStates::RenderTarget,
                    .FinalLayout = RHI::ResourceStates::ShaderResource
                },
                {
                    .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                    .Texture = this->NormalTex,
                    .InitialLayout = RHI::ResourceStates::ShaderResource,
                    .SubpassLayout = RHI::ResourceStates::RenderTarget,
                    .FinalLayout = RHI::ResourceStates::ShaderResource
                },
                {
                    .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                    .Texture = this->SurfaceTex,
                    .InitialLayout = RHI::ResourceStates::ShaderResource,
                    .SubpassLayout = RHI::ResourceStates::RenderTarget,
                    .FinalLayout = RHI::ResourceStates::ShaderResource
                },
                {
                    .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                    .Texture = this->SpecularTex,
                    .InitialLayout = RHI::ResourceStates::ShaderResource,
                    .SubpassLayout = RHI::ResourceStates::RenderTarget,
                    .FinalLayout = RHI::ResourceStates::ShaderResource
                },
                {
                    .Type = RenderPassAttachment::Type::DepthStencil,
                    .LoadOp = RenderPassAttachment::LoadOpType::Clear,
                    .Texture = this->DepthTex,
                    .InitialLayout = RHI::ResourceStates::DepthWrite,
                    .SubpassLayout = RHI::ResourceStates::DepthWrite,
                    .FinalLayout = RHI::ResourceStates::DepthWrite
                },
            }
        });
}

void PhxEngine::Renderer::GBufferRenderTargets::Free(RHI::IGraphicsDevice* gfxDevice)
{
    gfxDevice->DeleteRenderPass(this->RenderPass);
    gfxDevice->DeleteTexture(this->DepthTex);
    gfxDevice->DeleteTexture(this->AlbedoTex);
    gfxDevice->DeleteTexture(this->NormalTex);
    gfxDevice->DeleteTexture(this->SurfaceTex);
    gfxDevice->DeleteTexture(this->SpecularTex);
}
