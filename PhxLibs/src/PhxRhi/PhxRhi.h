#pragma once

#include <PhxRhi/RHICommon.h>

#include <PhxRhi/IBackend.h>
#include <PhxRhi/IResourceManager.h>
#include <PhxRhi/ISubmissionManager.h>

namespace phx::rhi
{
	bool Initialize(Descriptor const& descriptor, void* window_handle, size_t thread_count);
	bool Shutdown();
}
