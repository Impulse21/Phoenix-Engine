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

void SceneRenderLayer::OnRender()
{
    this->m_commandList->Open();
    this->m_commandList->TransitionBarrier(this->GetFinalColourBuffer(), this->GetFinalColourBuffer()->GetDesc().InitialState, RHI::ResourceStates::RenderTarget);
    {
        auto scrope = this->m_commandList->BeginScopedMarker("Clear Render Targets");

        this->m_commandList->ClearTextureFloat(this->GetFinalColourBuffer(), this->GetFinalColourBuffer()->GetDesc().OptmizedClearValue.value());
    }

    this->m_commandList->TransitionBarrier(this->GetFinalColourBuffer(), RHI::ResourceStates::RenderTarget, this->GetFinalColourBuffer()->GetDesc().InitialState);
    this->m_commandList->Close();
    RHI::IGraphicsDevice::Ptr->ExecuteCommandLists(this->m_commandList.get());
}

void SceneRenderLayer::ResizeSurface(DirectX::XMFLOAT2 const& size)
{
    // this->m_colourBuffers.clear();
    // this->CreateWindowTextures(size);
}

void SceneRenderLayer::CreateWindowTextures(DirectX::XMFLOAT2 const& size)
{
    RHI::TextureDesc desc = {};
    desc.IsBindless = true;
    desc.BindingFlags = RHI::BindingFlags::RenderTarget | RHI::BindingFlags::ShaderResource;
    desc.InitialState = RHI::ResourceStates::ShaderResource;
    desc.Width = std::max(size.x, 1.0f);
    desc.Height = std::max(size.y, 1.0f);

    RHI::Color clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
    desc.OptmizedClearValue = std::make_optional<RHI::Color>(clearValue);

    // TODO: Determine what format should be used here.
    desc.Format = RHI::FormatType::R10G10B10A2_UNORM;
    desc.DebugName = "Scene Colour Buffer";

    auto& spec = LayeredApplication::Ptr->GetSpec();
    for (size_t i = 0; i < spec.RendererConfig.FramesInFlight; i++)
    {
        this->m_colourBuffers.push_back(RHI::IGraphicsDevice::Ptr->CreateTexture(desc));
    }
}
