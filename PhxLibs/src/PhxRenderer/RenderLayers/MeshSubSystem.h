#pragma once

#include <PhxRenderer/RenderSubSystem.h>

namespace phx::gfx
{
	class MeshSubSystem final : public IRenderSubSystem
	{
	public:
		MeshSubSystem() = default;
		~MeshSubSystem() override = default;

		void* OnPreRender() override;
		void OnRender(rhi::CommandCtx* ctx, void* cachedData) override;
		void Finalize() override;

	};
}