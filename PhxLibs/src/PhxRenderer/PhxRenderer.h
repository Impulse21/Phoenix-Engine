#pragma once

#include <PhxRhi/PhxRhi_ForwardDeclares.h>

namespace phx::renderer
{
	void Initialize(rhi::IBackend* rhi_backend);
	void Shutdown();
}