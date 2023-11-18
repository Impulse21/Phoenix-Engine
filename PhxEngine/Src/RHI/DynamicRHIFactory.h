#pragma once

#include <PhxEngine/RHI/PhxRHIDefinitions.h>

namespace PhxEngine::RHI
{
	class DynamicRHI;

	class DynamicRHIFactory
	{
	public:
		static DynamicRHI* Create(RHI::GraphicsAPI preferedAPI);
	};
}