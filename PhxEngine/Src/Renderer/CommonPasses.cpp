#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/CommonPasses.h>



PhxEngine::Renderer::CommonPasses::CommonPasses(RHI::IGraphicsDevice* gfxDevice, Graphics::ShaderFactory& shaderFactory)
	: m_gfxDevice(gfxDevice)
{

	this->RectVS = shaderFactory.CreateShader(
		"RectVS.hlsl",
		{
			.Stage = RHI::ShaderStage::Vertex,
			.DebugName = "RectVS",
		});

	this->RectVS = shaderFactory.CreateShader(
		"BlitPS.hlsl",
		{
			.Stage = RHI::ShaderStage::Pixel,
			.DebugName = "BlitPS",
		});

	const uint32_t kBlackImage = 0xff000000;
	const uint32_t kWhiteImage = 0xffffffff;
	this->BlackTexture = this->m_gfxDevice->CreateTexture({
			.BindingFlags = RHI::BindingFlags::ShaderResource,
			.Dimension = RHI::TextureDimension::Texture2D,
			.InitialState = RHI::ResourceStates::CopyDest,
			.Format = RHI::RHIFormat::RGBA8_UNORM,
			.IsBindless = true,
			.Width = 1,
			.Height = 1,
			.DebugName = "BlackTexture",
		});

	this->WhiteTexture = this->m_gfxDevice->CreateTexture({
			.BindingFlags = RHI::BindingFlags::ShaderResource,
			.Dimension = RHI::TextureDimension::Texture2D,
			.InitialState = RHI::ResourceStates::CopyDest,
			.Format = RHI::RHIFormat::RGBA8_UNORM,
			.IsBindless = true,
			.Width = 1,
			.Height = 1,
			.DebugName = "WhiteTexture",
		});

	RHI::ICommandList* upload = this->m_gfxDevice->BeginCommandRecording();
	RHI::SubresourceData subResourceData = {};
	subResourceData.rowPitch = 0;
	subResourceData.slicePitch = 0;
	subResourceData.pData = &kBlackImage;

	upload->WriteTexture(this->BlackTexture, 0, 1, &subResourceData);

	subResourceData.pData = &kWhiteImage;
	upload->WriteTexture(this->BlackTexture, 0, 1, &subResourceData);

	RHI::GpuBarrier uploadBarriers[] =
	{
		RHI::GpuBarrier::CreateTexture(this->BlackTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
		RHI::GpuBarrier::CreateTexture(this->WhiteTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
	};
	upload->TransitionBarriers(Core::Span<RHI::GpuBarrier>(uploadBarriers, _countof(uploadBarriers)));

	upload->Close();
	this->m_gfxDevice->ExecuteCommandLists({ upload });
}

void PhxEngine::Renderer::CommonPasses::BlitTexture(RHI::ICommandList* cmdList, RHI::TextureHandle sourceTexture, RHI::RenderPassHandle renderPass, DirectX::XMFLOAT2 const& canvas)
{
	std::vector<RHI::RHIFormat> rtvs;
	RHI::RHIFormat depth;
	this->m_gfxDevice->GetRenderPassFormats(renderPass, rtvs, depth);
	PsoCacheKey key = { .RTVFormats = rtvs, .DepthFormat = depth };
	RHI::GraphicsPipelineHandle pso = this->m_psoCache[key];
	if (!pso.IsValid())
	{
		this->m_psoCache[key] = this->m_gfxDevice->CreateGraphicsPipeline({
			.VertexShader = this->RectVS,
			.PixelShader = this->BlitPS,
			.DepthStencilRenderState = {.DepthTestEnable = false, .StencilEnable = false },
			.RasterRenderState = {.CullMode = RHI::RasterCullMode::None },
			.RtvFormats = rtvs
			});
	}
	cmdList->BeginRenderPass(renderPass);

	cmdList->BindDynamicDescriptorTable(0, { sourceTexture });

	RHI::Viewport v(canvas.x, canvas.y);
	cmdList->SetViewports(&v, 1);

	RHI::Rect rec(LONG_MAX, LONG_MAX);
	cmdList->SetScissors(&rec, 1);
	cmdList->Draw(4);

	cmdList->EndRenderPass();
}
