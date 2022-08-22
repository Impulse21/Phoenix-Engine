#include "SceneRenderLayer.h"

using namespace PhxEngine;

SceneRenderLayer::SceneRenderLayer()
	: AppLayer("Scene Render Layer")
{
}

void SceneRenderLayer::OnAttach()
{
    auto& spec = LayeredApplication::Ptr->GetSpec();

    this->m_colourBuffers.reserve(spec.RendererConfig.FramesInFlight);

    this->CreateWindowTextures({ static_cast<float>(spec.WindowWidth), static_cast<float>(spec.WindowHeight) });
    this->m_commandList = RHI::IGraphicsDevice::Ptr->CreateCommandList();
}

void SceneRenderLayer::OnDetach()
{
    for (auto& handle : this->m_colourBuffers)
    {
        RHI::IGraphicsDevice::Ptr->FreeTexture(handle);
    }
}

void SceneRenderLayer::OnRender()
{
    this->m_commandList->Open();
    Core::Handle<RHI::Texture> renderTarget = this->GetFinalColourBuffer();
    const RHI::TextureDesc& renderTargetDesc = RHI::IGraphicsDevice::Ptr->GetTextureDesc(renderTarget);
    this->m_commandList->TransitionBarrier(renderTarget, renderTargetDesc.InitialState, RHI::ResourceStates::RenderTarget);
    {
        auto scrope = this->m_commandList->BeginScopedMarker("Clear Render Targets");

        this->m_commandList->ClearTextureFloat(this->GetFinalColourBuffer(), renderTargetDesc.OptmizedClearValue.Colour);
    }

    this->m_commandList->TransitionBarrier(renderTarget, RHI::ResourceStates::RenderTarget, renderTargetDesc.InitialState);
    this->m_commandList->Close();
    RHI::IGraphicsDevice::Ptr->ExecuteCommandLists(this->m_commandList.get());
}

void SceneRenderLayer::ResizeSurface(DirectX::XMFLOAT2 const& size)
{
    for (auto handle : this->m_colourBuffers)
    {
        RHI::IGraphicsDevice::Ptr->FreeTexture(handle);
    }

    this->m_colourBuffers.clear();
    this->CreateWindowTextures(size);
}

void SceneRenderLayer::CreateWindowTextures(DirectX::XMFLOAT2 const& size)
{
    RHI::TextureDesc desc = {};
    desc.IsBindless = true;
    desc.BindingFlags = RHI::BindingFlags::RenderTarget | RHI::BindingFlags::ShaderResource;
    desc.InitialState = RHI::ResourceStates::ShaderResource;
    desc.Width = std::max(size.x, 1.0f);
    desc.Height = std::max(size.y, 1.0f);

    // TODO: Fix a bug where clear value fails if colour other then red is set to 1
    desc.OptmizedClearValue.Colour = { 0.0f, 0.0f, 0.0f, 1.0f };

    // TODO: Determine what format should be used here.
    desc.Format = RHI::FormatType::R10G10B10A2_UNORM;
    // desc.Format = RHI::FormatType::RGBA32_FLOAT;
    desc.DebugName = "Scene Colour Buffer";

    auto& spec = LayeredApplication::Ptr->GetSpec();
    for (size_t i = 0; i < spec.RendererConfig.FramesInFlight; i++)
    {
        this->m_colourBuffers.push_back(RHI::IGraphicsDevice::Ptr->CreateTexture(desc));
    }
}
