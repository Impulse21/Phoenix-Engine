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
		phx::gfx::GfxDevice& device = phx::gfx::Ember::Ptr->GetDevice();

		this->m_pipeline = device.CreateGfxPipeline({
				.VertexShaderByteCode = phx::Span(g_pTestShaderVS, ARRAYSIZE(g_pTestShaderVS)),
				.PixelShaderByteCode = phx::Span(g_pTestShaderPS, ARRAYSIZE(g_pTestShaderPS)),
				.RtvFormats = { phx::gfx::g_SwapChainFormat }
			});
	};

	void Shutdonw() override 
	{
		phx::gfx::GfxDevice& device = phx::gfx::Ember::Ptr->GetDevice();

		device.DeleteGfxPipeline(this->m_pipeline);
	};

	void CacheRenderData() override {};
	void Update() override {};
	void Render() override
	{
		phx::gfx::GfxDevice& device = phx::gfx::Ember::Ptr->GetDevice();

		phx::gfx::CommandCtx command = device.BeginGfxContext();
		command.ClearBackBuffer({ 0.392156899f, 0.584313750f, 0.929411829f, 1.f  }); // Cornflower blue
	}

private:
	phx::gfx::GfxPipelineHandle m_pipeline;
};

CREATE_APPLICATION(PhxEditor)