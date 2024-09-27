//
// Main.cpp
//

#include "pch.h"

#include "phxEngineCore.h"
#include "EmberGfx/phxEmber.h"
#include "phxDisplay.h"

#include "CompiledShaders/TestShaderVS.h"
#include "CompiledShaders/TestShaderPS.h"

class PhxEditor final : public phx::IEngineApp
{
public:
	void Startup() override 
	{
		this->m_pipeline = phx::gfx::GfxDevice::CreateGfxPipeline({
				.VertexShaderByteCode = phx::Span(g_pTestShaderVS, ARRAYSIZE(g_pTestShaderVS)),
				.PixelShaderByteCode = phx::Span(g_pTestShaderPS, ARRAYSIZE(g_pTestShaderPS)),
				.RtvFormats = { phx::gfx::g_SwapChainFormat }
			});
	};

	void Shutdonw() override 
	{
		phx::gfx::GfxDevice::DeleteGfxPipeline(this->m_pipeline);
	};

	void CacheRenderData() override {};
	void Update() override {};
	void Render() override
	{
		phx::gfx::CommandCtx command = phx::gfx::GfxDevice::BeginGfxContext();
		command.ClearBackBuffer({ 0.392156899f, 0.584313750f, 0.929411829f, 1.f  }); // Cornflower blue
		command.SetGfxPipeline(this->m_pipeline);

	}

private:
	phx::gfx::GfxPipelineHandle m_pipeline;
};

CREATE_APPLICATION(PhxEditor)