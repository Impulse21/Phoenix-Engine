#pragma once

#include "EmberGfx/phxGfxDeviceResources.h"

namespace phx::gfx
{
	class ICommandCtx
	{
	public:
		virtual ~ICommandCtx() = default;

		virtual void RenderPassBegin() = 0;
		virtual void RenderPassEnd() = 0;

		virtual void SetPipelineState(PipelineStateHandle pipelineState) = 0;
		virtual void SetViewport(Viewport const& viewports) = 0;

		virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0) = 0;
		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0 , uint32_t startInstance = 0) = 0;
	};
}