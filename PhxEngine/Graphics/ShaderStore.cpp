#include "phxpch.h"
#include "ShaderStore.h"

#include "Graphics/ShaderFactory.h"


using namespace PhxEngine::Graphics;
using namespace PhxEngine::RHI;

void ShaderStore::PreloadShaders(ShaderFactory& shaderFactory)
{
	// TODO: Add Async

	this->m_shaderHandles.assign(static_cast<size_t>(PreLoadShaders::NumPreloadShaders), nullptr);

	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::VS_ImGui)] = shaderFactory.LoadShader(ShaderStage::Vertex, "ImGuiVS.cso");
	this->m_shaderHandles[static_cast<size_t>(PreLoadShaders::PS_ImGui)] = shaderFactory.LoadShader(ShaderStage::Pixel, "ImGuiPS.cso");

	return;
}

const ShaderHandle PhxEngine::Graphics::ShaderStore::Retrieve(PreLoadShaders shader) const
{
	size_t shaderIndex = static_cast<size_t>(shader);
	assert(shaderIndex < this->m_shaderHandles.size());

	return this->m_shaderHandles[shaderIndex];
}