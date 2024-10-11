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

		phx::gfx::ShaderCompiler::Output testShaderVSOutput = phx::gfx::ShaderCompiler::Compile({
			.Format = phx::gfx::ShaderFormat::Spriv,
				.ShaderStage = phx::gfx::ShaderStage::VS,
				.SourceFilename = "/native/Shaders/TestShader.hlsl",
				.EntryPoint = "MainVS",
				.FileSystem = m_fs.get()});

		if (!testShaderVSOutput.ErrorMessage.empty())
			PHX_ERROR("Failed to compile VS shader: {0}", testShaderVSOutput.ErrorMessage);

		phx::gfx::ShaderCompiler::Output testShaderPSOutput = phx::gfx::ShaderCompiler::Compile({
			.Format = phx::gfx::ShaderFormat::Spriv,
			.ShaderStage = phx::gfx::ShaderStage::PS,
			.SourceFilename = "/native/Shaders/TestShader.hlsl",
			.EntryPoint = "MainPS",
			.FileSystem = m_fs.get() });

		if (!testShaderPSOutput.ErrorMessage.empty())
			PHX_ERROR("Failed to compile PS shader: {0}", testShaderPSOutput.ErrorMessage);
		
		this->m_pipeline.Reset(
			phx::gfx::GfxDevice::CreateGfxPipeline({
				.VertexShaderByteCode = phx::Span(testShaderVSOutput.ByteCode, testShaderVSOutput.ByteCodeSize),
				.PixelShaderByteCode = phx::Span(testShaderPSOutput.ByteCode, testShaderPSOutput.ByteCodeSize),
				.DepthStencilRenderState = {.DepthTestEnable = false, .DepthWriteEnable = false },
				.RasterRenderState = { .CullMode = phx::gfx::RasterCullMode::None },
				.RtvFormats = { phx::gfx::g_SwapChainFormat }
			}));

		this->m_imguiRenderSystem.Initialize();
		this->m_imguiRenderSystem.EnableDarkThemeColours();
	};

	void Shutdown() override 
	{
		this->m_pipeline.Reset();
	};

	void CacheRenderData() override {};
	void Update() override 
	{
		PHX_EVENT();
		m_imguiRenderSystem.BeginFrame();
		phx::EngineProfile::DrawUI();
	};

	void Render() override
	{
		using namespace phx::gfx;
		phx::gfx::CommandCtx ctx = phx::gfx::GfxDevice::BeginGfxContext();
		PHX_EVENT_GFX(ctx);
		ctx.ClearBackBuffer({ 0.392156899f, 0.584313750f, 0.929411829f, 1.f  }); // Cornflower blue
		ctx.SetRenderTargetSwapChain();
		Viewport viewport(g_DisplayWidth, g_DisplayHeight);

		ctx.SetViewports({ &viewport, 1 });
		Rect rec(g_DisplayWidth, g_DisplayHeight);
		ctx.SetScissors({ &rec, 1 });
		ctx.SetGfxPipeline(this->m_pipeline);

		DynamicBuffer temp = ctx.AllocateDynamic(sizeof(uint16_t) * 3);
		
		uint16_t* indices = reinterpret_cast<uint16_t*>(temp.Data);
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;

		ctx.SetDynamicIndexBuffer(temp.BufferHandle, temp.Offset, 3, Format::R16_UINT);
		ctx.DrawIndexed(3, 1, 0, 0, 0);

		m_imguiRenderSystem.Render(ctx);
	}

private:
	phx::gfx::HandleOwner<phx::gfx::GfxPipeline> m_pipeline;
	phx::gfx::ImGuiRenderSystem m_imguiRenderSystem;
	std::unique_ptr<phx::IRootFileSystem> m_fs;
};

CREATE_APPLICATION(PhxEditor)