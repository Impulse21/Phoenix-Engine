#pragma once

#include <json.hpp>


#include <PhxEngine/RHI/PhxRHI.h>
namespace PhxEngine::Assets
{
	namespace AssetModule
	{
		void InitializeWindows(PhxEngine::RHI::GfxDevice* gfxDevice, nlohmann::json const& configuration);
		void Finalize();
	}
}

