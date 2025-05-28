#include "PhxRhi/PhxRhi_pch.h"

#include "PhxRhi/RHICommandCtx.h"
#include "PhxRhi/RHITypes.h"
#include "PhxRhi/RHICore.h"

#include "VkCore.h"


#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#endif

#define SAFE_DELETE(x) if (x) { delete x; }

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vk;

namespace
{
	const GUID kRenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
	const GUID kPixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

}

namespace phx::rhi::vk
{
	VkContext g_VkContext;
	size_t g_frameCount = 0;
}

namespace
{
	void RunGarbageCollection(uint64_t completedFrame)
	{
	}
}

namespace phx::rhi
{
	void Initialize(RhiCreateInfo const&)
	{
		PHX_CORE_INFO("Initialize RHI(Vulkan)");
	}

	void Finalize()
	{
		WaitForIdle();

	}


	Budget GetBudget()
	{
		return {};
	}

	void WaitForIdle()
	{
	}

	CommandCtx* BeginCommnadCtx(CommandQueueType queueType)
	{
		return nullptr;
	}

	void Present()
	{
		RunGarbageCollection(g_frameCount);
	}

	ShaderFormat GetShaderFormat() 
	{ 
		return ShaderFormat::Spriv;
	}
}