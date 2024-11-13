#pragma once

#include "phx/rhi/RHITypes.h"

namespace phx::rhi::dx12
{
	class GfxDeviceDx12
	{
	public:


		ShaderFormat GetShaderFormat() const { return ShaderFormat::Hlsl6; }
	};
}