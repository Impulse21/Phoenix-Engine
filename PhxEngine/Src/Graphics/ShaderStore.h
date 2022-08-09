#pragma once

#include "PhxEngine/Graphics/RHI/PhxRHI.h"

namespace PhxEngine::Graphics
{
	enum class PreLoadShaders
	{
		// VS Shaders
		VS_ImGui,
		VS_GBufferPass,
		VS_FullscreenQuad,
		VS_DeferredLighting,
		VS_ShadowPass,

		// PS Shaders
		PS_ImGui,
		PS_GBufferPass,
		PS_FullscreenQuad,
		PS_DeferredLighting,

		// Compute Shaders
		CS_DeferredLighting,

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

