#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/ToneMappingPass.h>
#include <PhxEngine/Renderer/CommonPasses.h>
#include <PhxEngine/Graphics/ShaderFactory.h>

using namespace PhxEngine::Renderer;

namespace RootParameters
{
	enum
	{
		Push = 0,
		HDRBuffer,
	};
}

PhxEngine::Renderer::ToneMappingPass::ToneMappingPass(
	RHI::IGraphicsDevice* gfxDevice,
	std::shared_ptr<Renderer::CommonPasses> commonPasses)
	: m_gfxDevice(gfxDevice)
	, m_commonPasses(commonPasses)
{
}

void PhxEngine::Renderer::ToneMappingPass::Initialize(Graphics::ShaderFactory& factory)
{
    this->m_pixelShader = factory.CreateShader(
        "PhxEngine/ToneMappingPS.hlsl",
        {
            .Stage = RHI::ShaderStage::Pixel,
            .DebugName = "ToneMappingPS",
        });
}

void PhxEngine::Renderer::ToneMappingPass::OnWindowResize()
{
    this->m_psoCache.clear();
}

void PhxEngine::Renderer::ToneMappingPass::Render(RHI::ICommandList* commandList, RHI::TextureHandle sourceTexture, RHI::RenderPassHandle renderPass, DirectX::XMFLOAT2 const& canvas)
{
	std::vector<RHI::RHIFormat> rtvs;
	RHI::RHIFormat depth;
	this->m_gfxDevice->GetRenderPassFormats(renderPass, rtvs, depth);
	PsoCacheKey key = { .RTVFormats = { RHI::RHIFormat::R10G10B10A2_UNORM } , .DepthFormat = depth };
	RHI::GraphicsPipelineHandle pso = this->m_psoCache[key];
	if (!pso.IsValid())
	{
		this->m_psoCache[key] = this->m_gfxDevice->CreateGraphicsPipeline({
			.PrimType = RHI::PrimitiveType::TriangleStrip,
			.VertexShader = this->m_commonPasses->RectVS,
			.PixelShader = this->m_pixelShader,
			.DepthStencilRenderState = {.DepthTestEnable = false, .StencilEnable = false },
			.RasterRenderState = {.CullMode = RHI::RasterCullMode::None },
			.RtvFormats = { RHI::RHIFormat::R10G10B10A2_UNORM }
			});
	}

	auto scopedMarker = commandList->BeginScopedMarker("Tone Map");
	commandList->BeginRenderPass(renderPass);
	commandList->SetGraphicsPipeline(this->m_psoCache[key]);
	commandList->BindDynamicDescriptorTable(RootParameters::HDRBuffer, { sourceTexture });
	commandList->BindPushConstant(RootParameters::Push, 1.0f);

	RHI::Viewport v(canvas.x, canvas.y);
	commandList->SetViewports(&v, 1);

	RHI::Rect rec(LONG_MAX, LONG_MAX);
	commandList->SetScissors(&rec, 1);
	commandList->Draw(3);

	commandList->EndRenderPass();
}
