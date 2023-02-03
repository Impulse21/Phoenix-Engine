#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/CommonPasses.h>



PhxEngine::Renderer::CommonPasses::CommonPasses(RHI::IGraphicsDevice* gfxDevice, Graphics::ShaderFactory& shaderFactory)
	: m_gfxDevice(gfxDevice)
{

	this->RectVS = shaderFactory.LoadShader(
		"RectVS.hlsl",
		{
			.Stage = RHI::ShaderStage::Vertex,
			.DebugName = "RectVS",
		});

	this->RectVS = shaderFactory.LoadShader(
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

void PhxEngine::Renderer::CommonPasses::BlitTexture(RHI::ICommandList* cmdList, RHI::TextureHandle sourceTexture, RHI::RenderPassHandle renderPass)
{
	RHI::GraphicsPipelineHandle gfxPipeline = this->m_psoCache[PsoCacheKey{ fbinfo, shader, params.blendState }];
	if ()
	{

	}
	cmdList->BeginRenderPass(renderPass);

	cmdList->BindDynamicDescriptorTable(0, { sourceTexture });
}
