//
// Main.cpp
//

#include "pch.h"

#include "phxEngineCore.h"
#include "EmberGfx/phxEmber.h"
#include "phxEngineProfiler.h"

#include "phxDisplay.h"

#include "CompiledShaders/TestShaderVS.h"
#include "CompiledShaders/TestShaderPS.h"
#include "EmberGfx/phxImGuiRenderer.h"

class PhxEditor final : public phx::IEngineApp
{
public:
	void Startup() override 
	{
		this->m_pipeline.Reset(
			phx::gfx::GfxDevice::CreateGfxPipeline({
				.VertexShaderByteCode = phx::Span(g_pTestShaderVS, ARRAYSIZE(g_pTestShaderVS)),
				.PixelShaderByteCode = phx::Span(g_pTestShaderPS, ARRAYSIZE(g_pTestShaderPS)),
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
};

CREATE_APPLICATION(PhxEditor)