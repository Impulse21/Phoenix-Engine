#pragma once

#include "phxGfxCore.h"

namespace phx::gfx
{
	class D3D12Renderer final : public Renderer
	{
	public:
		D3D12Renderer() = default;
		~D3D12Renderer() = default;

	public:
		void SubmitCommands() override;
	};
}