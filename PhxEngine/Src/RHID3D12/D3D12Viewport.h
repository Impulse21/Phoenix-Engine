#pragma once

#include <PhxEngine/RHI/PhxRHI.h>

#include "D3D12Common.h"

namespace PhxEngine::RHI::D3D12
{
	class D3D12Viewport : public RefCounter<IRHIViewport>, public D3D12AdapterChild
	{
	public:
		D3D12Viewport(D3D12Adapter* parent = nullptr) 
			: D3D12AdapterChild(parent) {}
		~D3D12Viewport() = default;
	};
}

