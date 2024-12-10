#pragma once

#include "Dx12DeviceResources.h"

namespace phx::rhi::dx12
{
    class GfxCommandListRecorder
    {
    public:
        void Open(CommandListResource* resource);
        void Close(CommandListResource* resource)
        {
            resource->CmdList->Close();
        }

		void RenderPassBegin(CommandListResource* resource, SwapChainBindings* bindings)
		{
		}

		void RenderPassEnd(CommandListResource* resource)
		{

		}

		void SetViewports(CommandListResource* resource, phx::Span<Viewport> viewports)
		{
			UNREFERENCED_PARAMETER(viewports);
		}

		void SetScissors(CommandListResource* resource, phx::Span<Rect> scissors)
		{
			UNREFERENCED_PARAMETER(scissors);

		}

		void SetPipelineState(CommandListResource* resource, PipelineStateHandle handle)
		{
			UNREFERENCED_PARAMETER(handle);

		}

		void DrawIndexed(CommandListResource* resource, uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0)
		{
			UNREFERENCED_PARAMETER(indexCount);
			UNREFERENCED_PARAMETER(instanceCount);
			UNREFERENCED_PARAMETER(startIndex);
			UNREFERENCED_PARAMETER(baseVertex);
			UNREFERENCED_PARAMETER(startInstance);

		}

		void Draw(CommandListResource* resource, uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0)
		{
			UNREFERENCED_PARAMETER(vertexCount);
			UNREFERENCED_PARAMETER(instanceCount);
			UNREFERENCED_PARAMETER(startVertex);
			UNREFERENCED_PARAMETER(startInstance);
		}

		void SetDynamicVertexBuffer(CommandListResource* resource, GpuBufferHandle tempBuffer, size_t offset, uint32_t slot, size_t numVertices, size_t vertexSize)
		{
			UNREFERENCED_PARAMETER(tempBuffer);
			UNREFERENCED_PARAMETER(offset);
			UNREFERENCED_PARAMETER(slot);
			UNREFERENCED_PARAMETER(numVertices);
			UNREFERENCED_PARAMETER(vertexSize);
		}

		void SetDynamicIndexBuffer(CommandListResource* resource, GpuBufferHandle tempBuffer, size_t offset, size_t numIndicies, Format indexFormat)
		{
			UNREFERENCED_PARAMETER(tempBuffer);
			UNREFERENCED_PARAMETER(offset);
			UNREFERENCED_PARAMETER(numIndicies);
			UNREFERENCED_PARAMETER(indexFormat);
		}

		void SetPushConstant(CommandListResource* resource, uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants)
		{
			UNREFERENCED_PARAMETER(rootParameterIndex);
			UNREFERENCED_PARAMETER(sizeInBytes);
			UNREFERENCED_PARAMETER(constants);
		}
    };
}