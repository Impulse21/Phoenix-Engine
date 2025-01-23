#include "phxpch.h"

#include "phx/rhi/RHICore.h"

#include "D3D12Core.h"

namespace phx::rhi::d3d12
{
	phx::ResourcePool<rhi::PipelineState, PipelineStateResource> g_pipelineStatePool;
	phx::ResourcePool<rhi::Texture, TextureBindings, TextureResource> g_texturePool;
}


namespace phx::rhi::d3d12
{
	void InitializeResources(rhi::RhiCreateInfo const& createInfo)
	{
		g_pipelineStatePool.Initialize(createInfo.MaxNumTextures);
		g_texturePool.Initialize(createInfo.MaxNumPipelineStates);
	}

}