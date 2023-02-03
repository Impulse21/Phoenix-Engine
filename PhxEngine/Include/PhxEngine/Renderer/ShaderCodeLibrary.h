#pragma once

#include <PhxEngine/RHI/PhxRHI.h>

namespace PhxEngine::Renderer
{
	class ShaderCodeLibrary
	{
	public:
		ShaderCodeLibrary() = default;
		~ShaderCodeLibrary() = default;

		RHI::ShaderHandle CreateShader(RHI::ShaderDesc const desc);
	
	};
}