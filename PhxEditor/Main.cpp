//
// Main.cpp
//

#include "pch.h"

#include "phxEngineCore.h"
#include "phxGfx/phxGfxCore.h"

class PhxEditor final : public phx::IEngineApp
{
public:

	void Startup() override {};
	void Shutdonw() override {};

	void CacheRenderData() override {};
	void Update() override {};
	void Render() override
	{
	}
};

CREATE_APPLICATION(PhxEditor)