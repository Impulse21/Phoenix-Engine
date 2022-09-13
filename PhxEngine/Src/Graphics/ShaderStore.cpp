#include "phxpch.h"
#include <PhxEngine/Graphics/ShaderStore.h>

#include <PhxEngine/Graphics/ShaderFactory.h>


using namespace PhxEngine::Graphics;
using namespace PhxEngine::RHI;

void ShaderStore::PreloadShaders(ShaderFactory& shaderFactory)
{
	// TODO: Add Async

	this->m_shaderHandles.assign(static_cast<size_t>(PreLoadShaders::NumPreloadShaders), nullptr);

	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::VS_ImGui)] = shaderFactory.LoadShader(ShaderStage::Vertex, "ImGuiVS.cso");
	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::PS_ImGui)] = shaderFactory.LoadShader(ShaderStage::Pixel, "ImGuiPS.cso");

	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::VS_GBufferPass)] = shaderFactory.LoadShader(ShaderStage::Vertex, "GBufferPassVS.cso");
	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::PS_GBufferPass)] = shaderFactory.LoadShader(ShaderStage::Pixel, "GBufferPassPS.cso");

	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::VS_FullscreenQuad)] = shaderFactory.LoadShader(ShaderStage::Vertex, "FullScreenQuadVS.cso");
	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::PS_FullscreenQuad)] = shaderFactory.LoadShader(ShaderStage::Pixel, "FullScreenQuadPS.cso");

	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::VS_DeferredLighting)] = shaderFactory.LoadShader(ShaderStage::Vertex, "DeferredLightingVS.cso");
	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::PS_DeferredLighting)] = shaderFactory.LoadShader(ShaderStage::Pixel, "DeferredLightingPS.cso");

	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::CS_DeferredLighting)] = shaderFactory.LoadShader(ShaderStage::Compute, "DeferredLightingCS.cso");

	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::VS_ToneMapping)] = shaderFactory.LoadShader(ShaderStage::Vertex, "ToneMappingVS.cso");
	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::PS_ToneMapping)] = shaderFactory.LoadShader(ShaderStage::Pixel, "ToneMappingPS.cso");

	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::VS_ShadowPass)] = shaderFactory.LoadShader(ShaderStage::Vertex, "ShadowPassVS.cso");


	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::VS_Sky)] = shaderFactory.LoadShader(ShaderStage::Vertex, "SkyVS.cso");
	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::PS_SkyProcedural)] = shaderFactory.LoadShader(ShaderStage::Pixel, "Sky_ProceduralPS.cso");

	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::VS_EnvMap_Sky)] = shaderFactory.LoadShader(ShaderStage::Vertex, "EnvMap_SkyVS.cso");
	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::PS_EnvMap_SkyProcedural)] = shaderFactory.LoadShader(ShaderStage::Pixel, "EnvMap_Sky_ProceduralPS.cso");

	return;
}

const ShaderHandle PhxEngine::Graphics::ShaderStore::Retrieve(PreLoadShaders shader) const
{
	size_t shaderIndex = static_cast<size_t>(shader);
	assert(shaderIndex < this->m_shaderHandles.size());

	return this->m_shaderHandles[shaderIndex];
}