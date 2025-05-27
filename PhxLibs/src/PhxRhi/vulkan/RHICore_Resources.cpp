#include "PhxRhi/PhxRhi_pch.h"

#include <deque>

#include "PhxCore/EnumUtils.h"
#include "PhxCore/StringUtils.h"
#include "PhxRhi/RHICore.h"

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vk;

namespace
{
}

namespace phx::rhi::vk
{
	void InitializeResources(rhi::RhiCreateInfo const& createInfo)
	{
	}

	void FinalizeResources()
	{
	}

}

namespace phx::rhi
{
	TextureHandle CreateTexture(TextureDescriptor const& desc, MemInfo* initData)
	{ 
		return {};
	}

	PipelineStateHandle CreatePipelineState(PipelineStateDescriptor const& desc)
	{
		return {};
	}

	GpuBufferHandle CreateBuffer(GpuBufferDescriptor const& desc, MemInfo* initData)
	{
		return {};
	}

	void DeletePipeline(PipelineStateHandle handle)
	{
		vk::EnqueueDelete({
				g_frameCount,
				[=]()
				{
				}
			});
	}

	void DeleteTexture(TextureHandle handle)
	{
		vk::EnqueueDelete({
				g_frameCount,
				[=]()
				{
				}
			});
	}

	void DeleteBuffer(GpuBufferHandle /*handle*/)
	{

	}

	DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type)
	{
			return rhi::cInvalidDescriptorIndex;
	}
}
