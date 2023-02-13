#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/CommonPasses.h>



PhxEngine::Renderer::CommonPasses::CommonPasses(RHI::IGraphicsDevice* gfxDevice, Graphics::ShaderFactory& shaderFactory)
	: m_gfxDevice(gfxDevice)
{

	this->RectVS = shaderFactory.CreateShader(
		"PhxEngine/RectVS.hlsl",
		{
			.Stage = RHI::ShaderStage::Vertex,
			.DebugName = "RectVS",
		});

	this->BlitPS = shaderFactory.CreateShader(
		"PhxEngine/BlitPS.hlsl",
		{
			.Stage = RHI::ShaderStage::Pixel,
			.DebugName = "BlitPS",
		});

	this->BlackTexture = this->m_gfxDevice->CreateTexture({
			.BindingFlags = RHI::BindingFlags::ShaderResource,
			.Dimension = RHI::TextureDimension::Texture2D,
			.InitialState = RHI::ResourceStates::CopyDest,
			.Format = RHI::RHIFormat::BC1_UNORM,
			.IsBindless = true,
			.Width = 1,
			.Height = 1,
			.DebugName = "BlackTexture",
		});

	this->WhiteTexture = this->m_gfxDevice->CreateTexture({
			.BindingFlags = RHI::BindingFlags::ShaderResource,
			.Dimension = RHI::TextureDimension::Texture2D,
			.InitialState = RHI::ResourceStates::CopyDest,
			.Format = RHI::RHIFormat::BC1_UNORM,
			.IsBindless = true,
			.Width = 1,
			.Height = 1,
			.DebugName = "WhiteTexture",
		});

	RHI::ICommandList* upload = this->m_gfxDevice->BeginCommandRecording();

	const uint32_t texWidth = 4;
	const uint32_t texHeight = 4;
	const uint32_t textureSize = texWidth * texHeight;

	RHI::SubresourceData subResourceData = {};
	subResourceData.rowPitch = (texWidth + 3) / 4 * 8;
	subResourceData.slicePitch = 0;

	std::array<uint8_t, textureSize> blackImage;
	std::memset(blackImage.data(), 0, blackImage.size());
	subResourceData.pData = blackImage.data();
	upload->WriteTexture(this->BlackTexture, 0, 1, &subResourceData);

	std::array<uint8_t, textureSize> whiteImage;
	std::memset(whiteImage.data(), 0xff, whiteImage.size());
	subResourceData.pData = whiteImage.data();
	upload->WriteTexture(this->WhiteTexture, 0, 1, &subResourceData);


	RHI::GpuBarrier uploadBarriers[] =
	{
		RHI::GpuBarrier::CreateTexture(this->BlackTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
		RHI::GpuBarrier::CreateTexture(this->WhiteTexture, RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
	};
	upload->TransitionBarriers(Core::Span<RHI::GpuBarrier>(uploadBarriers, _countof(uploadBarriers)));

	upload->Close();
	this->m_gfxDevice->ExecuteCommandLists({ upload }, true);
}

void PhxEngine::Renderer::CommonPasses::BlitTexture(RHI::ICommandList* cmdList, RHI::TextureHandle sourceTexture, RHI::RenderPassHandle renderPass, DirectX::XMFLOAT2 const& canvas)
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
			.VertexShader = this->RectVS,
			.PixelShader = this->BlitPS,
			.DepthStencilRenderState = {.DepthTestEnable = false, .StencilEnable = false },
			.RasterRenderState = {.CullMode = RHI::RasterCullMode::None },
			.RtvFormats = { RHI::RHIFormat::R10G10B10A2_UNORM }
			});
	}
	auto scopedMarker = cmdList->BeginScopedMarker("Blit Texture");
	cmdList->BeginRenderPass(renderPass);
	cmdList->SetGraphicsPipeline(this->m_psoCache[key]);
	cmdList->BindDynamicDescriptorTable(0, { sourceTexture });

	RHI::Viewport v(canvas.x, canvas.y);
	cmdList->SetViewports(&v, 1);

	RHI::Rect rec(LONG_MAX, LONG_MAX);
	cmdList->SetScissors(&rec, 1);
	cmdList->Draw(3);

	cmdList->EndRenderPass();
}
