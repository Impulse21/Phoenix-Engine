//
// Main.cpp
//

#include "pch.h"

#include "phxEngineCore.h"
#include "EmberGfx/phxEmber.h"

#include "CompiledShaders/TestShaderVS.h"
#include "CompiledShaders/TestShaderPS.h"

class PhxEditor final : public phx::IEngineApp
{
public:
	void Startup() override 
	{

	};

	void Shutdonw() override {};

	void CacheRenderData() override {};
	void Update() override {};
	void Render() override
	{
		phx::gfx::GfxDevice& device = phx::gfx::Ember::Ptr->GetDevice();

		phx::gfx::CommandCtx command = device.BeginGfxContext();
		command.ClearBackBuffer({ 1.0, 0.0f, 0.0, 1.0f });
	}

private:
	phx::gfx::GfxPipelineHandle m_pipeline;
};

CREATE_APPLICATION(PhxEditor)