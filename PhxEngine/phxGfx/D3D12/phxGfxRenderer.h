#pragma once

#include "phxGfx.h"

namespace phx::gfx
{
	class D3D12Renderer : public Renderer
	{
	public:
		D3D12Renderer() = default;
		~D3D12Renderer() = default;
	};
}