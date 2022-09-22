#pragma once

#include "PhxEngine/Graphics/RHI/PhxRHI.h"

namespace PhxEngine::Graphics
{
	enum class PreLoadShaders
	{
		// -- VS Shaders ---
		VS_ImGui,
		VS_GBufferPass,
		VS_FullscreenQuad,
		VS_DeferredLighting,
		VS_ShadowPass,
		VS_ToneMapping,
		VS_Sky,
		VS_EnvMap_Sky,

		// -- PS Shaders ---
		PS_ImGui,
		PS_GBufferPass,
		PS_FullscreenQuad,
		PS_DeferredLighting,
		PS_ToneMapping,
		PS_SkyProcedural,
		PS_EnvMap_SkyProcedural,

		// -- Compute Shaders ---
		CS_DeferredLighting,
		CS_GenerateMips_TextureCubeArray,
		CS_FilterEnvMap,

		NumPreloadShaders
	};

	class ShaderFactory;
	class ShaderStore
	{
	public:
		inline static ShaderStore* Ptr = nullptr;
		void PreloadShaders(ShaderFactory& shaderFactory);

	public:
		// Known Shader getters
		const RHI::ShaderHandle Retrieve(PreLoadShaders shader) const;

	private:
		// Shader Registry
		std::vector<PhxEngine::RHI::ShaderHandle> m_shaderHandles;
	};
}

