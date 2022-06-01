#pragma once

#include "Graphics/RHI/PhxRHI.h"

namespace PhxEngine::Graphics
{
	enum class PreLoadShaders
	{
		// VS Shaders
		VS_ImGui,

		// PS Shaders
		PS_ImGui,

		NumPreloadShaders
	};

	class ShaderFactory;
	class ShaderStore
	{
	public:
		void PreloadShaders(ShaderFactory& shaderFactory);

	public:
		// Known Shader getters
		const RHI::ShaderHandle Retrieve(PreLoadShaders shader) const;

	private:
		// Shader Registry
		std::vector<PhxEngine::RHI::ShaderHandle> m_shaderHandles;
	};
}

