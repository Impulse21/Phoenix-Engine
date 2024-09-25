//
// Main.cpp
//

#include "pch.h"

#include "phxEngineCore.h"
#include "EmberGfx/phxEmber.h"

class PhxEditor final : public phx::IEngineApp
{
public:
	void Startup() override {};
	void Shutdonw() override {};

	void CacheRenderData() override {};
	void Update() override {};
	void Render() override
	{
		phx::gfx::GfxDevice* device = phx::gfx::Ember::Ptr->GfxDevice;

		phx::gfx::CommandList& command = device->BeginGfxContext();
		command.ClearBackBuffer({ 1.0, 0.0f, 0.0, 1.0f });
	}
};

CREATE_APPLICATION(PhxEditor)