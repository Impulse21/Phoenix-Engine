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
	}
};

CREATE_APPLICATION(PhxEditor)