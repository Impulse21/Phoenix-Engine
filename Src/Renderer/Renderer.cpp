#include <PhxEngine/Renderer/Renderer.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Renderer/Scene.h>

// TODO: Create a proper Descriptor Table abstraction at some point.
#include <PhxEngine/RHI/Dx12/RootSignatureBuilder.h>
#include <Shaders/ShaderInterop.h>

#include "ShaderDump.h"

using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;

void FrustrumCull(CameraComponent const& cameraComponent, New::Scene const& scene, CullResults& results)
{
	results.VisibleMeshInstance.clear();

	// TODO: Add tests
	for (int i = 0; i < scene.MeshInstances.GetCount(); i++)
	{
		results.VisibleMeshInstance.emplace_back(i);
	}
}

void PhxEngine::Renderer::RenderSystem::DrawScene(Scene const& scene, CullResults const& cullResults, RenderPass renderPass, RHI::CommandListHandle commandList)
{
}

void PhxEngine::Renderer::RenderSystem::DrawSky()
{
}

void PhxEngine::Renderer::RenderSystem::FrustrumCull(CameraComponent const& cameraComponent, New::Scene const& scene, CullResults& results)
{
}

ShaderHandle RenderSystem::LoadShader(EShaderType shaderType, std::string const& filename)
{
    auto it = sShaderDump.find(filename);
    if (it != sShaderDump.end())
    {
        ShaderDesc desc = {};
        desc.DebugName = filename;
        desc.ShaderType = shaderType;

        return this->m_graphicsDevice.CreateShader(desc, std::get<0>(it->second), std::get<1>(it->second));
    }
    else
    {
        LOG_CORE_ERROR("Shader dump doesn't contain shader: '%s'", filename);
    }
}


void PhxEngine::Renderer::RenderSystem::InitializeBindlessDescriptorTable()
{
    this->m_bindlessDescriptorTable = {};
    auto range = this->m_graphicsDevice.GetNumBindlessDescriptors();
    this->m_bindlessDescriptorTable.AddSRVRange<0, 100>(
        range,
        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
        0);

    this->m_bindlessDescriptorTable.AddSRVRange<0, 101>(
        range,
        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
        0);

    this->m_bindlessDescriptorTable.AddSRVRange<0, 102>(
        range,
        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
        0);
}

void PhxEngine::Renderer::FillBindlessDescriptorTable(IGraphicsDevice* graphicsDevice, RHI::Dx12::DescriptorTable& table)
{
    auto range = graphicsDevice->GetNumBindlessDescriptors();
    table.AddSRVRange<0, 100>(
        range,
        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
        0);

    table.AddSRVRange<0, 101>(
        range,
        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
        0);

    table.AddSRVRange<0, 102>(
        range,
        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
        0);
}
