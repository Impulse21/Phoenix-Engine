//
// Main.cpp
//

#include "pch.h"

#include "phxEngineCore.h"
#include "EmberGfx/phxEmber.h"
#include "phxEngineProfiler.h"

#include "phxDisplay.h"

#include "EmberGfx/phxImGuiRenderer.h"
#include "EmberGfx/phxShaderCompiler.h"
#include "phxVFS.h"

class PhxEditor final : public phx::IEngineApp
{
public:
	void Startup() override 
	{
		m_fs = phx::FileSystemFactory::CreateRootFileSystem();
		m_fs->Mount("/native", phx::FileSystemFactory::CreateNativeFileSystem());
		m_fs->Mount("/shaders", "/shaders/spriv");

		phx::gfx::GpuDevice* device = phx::gfx::EmberGfx::GetDevice();

		phx::gfx::ShaderCompiler::Output testShaderVSOutput = phx::gfx::ShaderCompiler::Compile({
			.Format = phx::gfx::ShaderFormat::Spriv,
				.ShaderStage = phx::gfx::ShaderStage::VS,
				.SourceFilename = "/native/Shaders/TestShader.hlsl",
				.EntryPoint = "MainVS",
				.FileSystem = m_fs.get()});

		m_fs->WriteFile("/shaders/TestShaderVS.spriv", phx::Span((char*)testShaderVSOutput.ByteCode, testShaderVSOutput.ByteCodeSize));
		phx::gfx::ShaderHandle vsShader = device->CreateShader({
				.Stage = phx::gfx::ShaderStage::VS,
				.ByteCode = phx::Span(testShaderVSOutput.ByteCode, testShaderVSOutput.ByteCodeSize),
				.EntryPoint = "MainVS"
			});

		phx::gfx::ShaderCompiler::Output testShaderPSOutput = phx::gfx::ShaderCompiler::Compile({
			.Format = phx::gfx::ShaderFormat::Spriv,
			.ShaderStage = phx::gfx::ShaderStage::PS,
			.SourceFilename = "/native/Shaders/TestShader.hlsl",
			.EntryPoint = "MainPS",
			.FileSystem = m_fs.get() });

		m_fs->WriteFile("/shaders/TestShaderPS.spriv", phx::Span((char*)testShaderPSOutput.ByteCode, testShaderPSOutput.ByteCodeSize));

		phx::gfx::ShaderHandle psShader = device->CreateShader({
				.Stage = phx::gfx::ShaderStage::PS,
				.ByteCode = phx::Span(testShaderPSOutput.ByteCode, testShaderPSOutput.ByteCodeSize),
				.EntryPoint = "MainPS"
			});

		phx::gfx::InputLayout il = {
			.elements = {
				{
					.SemanticName = "POSITION",
					.SemanticIndex = 0,
					.Format = phx::gfx::Format::RG32_FLOAT,
				},
				{
					.SemanticName = "COLOR",
					.SemanticIndex = 1,
					.Format = phx::gfx::Format::RGB32_FLOAT,
				}
			}
		};

		phx::gfx::DepthStencilRenderState dss = { .DepthEnable = false, .DepthWriteMask = phx::gfx::DepthWriteMask::Zero};
		phx::gfx::RasterRenderState rs = {.CullMode = phx::gfx::RasterCullMode::None };

		phx::gfx::RenderPassInfo passInfo;
		passInfo.RenderTargetCount = 1;
		passInfo.RenderTargetFormats[0] = phx::gfx::g_SwapChainFormat;

		this->m_pipeline = device->CreatePipeline({
				.VS = vsShader,
				.PS = psShader,
				.DepthStencilRenderState = &dss,
				.RasterRenderState = &rs,
				.InputLayout = nullptr// &il
			},
			&passInfo);

		device->DeleteShader(vsShader);
		device->DeleteShader(psShader);

		// this->m_imguiRenderSystem.Initialize();
		// this->m_imguiRenderSystem.EnableDarkThemeColours();
	};

	void Shutdown() override 
	{
		phx::gfx::GpuDevice* device = phx::gfx::EmberGfx::GetDevice();
		device->DeletePipeline(this->m_pipeline);
	};

	void CacheRenderData() override {};
	void Update() override 
	{
		PHX_EVENT();
		// m_imguiRenderSystem.BeginFrame();
		phx::EngineProfile::DrawUI();
	};

	void Render() override
	{
		using namespace phx::gfx;
		GpuDevice* device = EmberGfx::GetDevice();
		CommandCtx* ctx = device->BeginCommandCtx();

		ctx->RenderPassBegin();

		Viewport v(g_DisplayWidth, g_DisplayHeight);
		ctx->SetViewports({ v });

		Rect scissor(g_DisplayWidth, g_DisplayHeight);
		ctx->SetScissors({ scissor });
		ctx->SetPipelineState(m_pipeline);
#if false
		DynamicBuffer temp = ctx->AllocateDynamic(sizeof(uint16_t) * 3);

		uint16_t* indices = reinterpret_cast<uint16_t*>(temp.Data);
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;

		ctx->SetDynamicIndexBuffer(temp.BufferHandle, temp.Offset, 3, Format::R16_UINT);

		ctx->DrawIndexed(3);
#else

		ctx->Draw(3);
#endif

		ctx->RenderPassEnd();
	}

private:
	phx::gfx::PipelineStateHandle m_pipeline;
	phx::gfx::ImGuiRenderSystem m_imguiRenderSystem;
	std::unique_ptr<phx::IRootFileSystem> m_fs;
};

CREATE_APPLICATION(PhxEditor)