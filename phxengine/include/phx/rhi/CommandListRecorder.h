#pragma once

#include "RHITypes.h"
#include "PlatformTypes.h"
#include "GfxDevice.h"

namespace phx::rhi
{
	class GfxDevice;

	class GfxCommandListRecorder
	{
	public:
		static GfxCommandListRecorder Begin(GfxDevice* device, CommandListHandle cmdHandle)
		{
			return GfxCommandListRecorder(device, cmdHandle);
		}

	public:

		void Finished()
		{
			m_platformRecorder.Close();
		}

		void RenderPassBegin(SwapChainHandle handle)
		{
			auto* binding = m_device->GetSwapChainPool().Get<platform::SwapChainBindings>(handle);
			m_platformRecorder.RenderPassBegin(binding);
		}

		void RenderPassEnd()
		{
			m_platformRecorder.RenderPassEnd();
		}

		void SetViewports(phx::Span<Viewport> viewports)
		{
			m_platformRecorder.SetViewports(viewports);
		}

		void SetScissors(phx::Span<Rect> scissors)
		{
			m_platformRecorder.SetScissors(scissors);

		}

		void SetPipelineState(PipelineStateHandle handle)
		{
			auto* resource = m_device->GetPipelineStatePool().Get<platform::PipelineStateResource>(handle);
			m_platformRecorder.SetPipelineState(resource);
		}

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0)
		{
			m_platformRecorder.DrawIndexed(indexCount, instanceCount, startIndex, baseVertex,  startInstance);
		}

		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0)
		{
			m_platformRecorder.Draw(vertexCount, instanceCount, startVertex, startInstance);
		}

		void SetDynamicVertexBuffer(GpuBufferHandle tempBuffer, size_t offset, uint32_t slot, size_t numVertices, size_t vertexSize)
		{
			UNREFERENCED_PARAMETER(tempBuffer);
			UNREFERENCED_PARAMETER(offset);
			UNREFERENCED_PARAMETER(slot);
			UNREFERENCED_PARAMETER(numVertices);
			UNREFERENCED_PARAMETER(vertexSize);
		}

		void SetDynamicIndexBuffer(GpuBufferHandle tempBuffer, size_t offset, size_t numIndicies, Format indexFormat)
		{
			UNREFERENCED_PARAMETER(tempBuffer);
			UNREFERENCED_PARAMETER(offset);
			UNREFERENCED_PARAMETER(numIndicies);
			UNREFERENCED_PARAMETER(indexFormat);
		}

		void SetPushConstant(uint32_t rootParameterIndex, size_t sizeInBytes, const void* constants)
		{
			m_platformRecorder.SetPushConstant(rootParameterIndex, sizeInBytes, constants);
		}

		template<typename T>
		void SetPushConstant(uint32_t rootParameterIndex, T const& constants)
		{
			SetPushConstant(rootParameterIndex, sizeof(T), &constants);
		}

	protected:
		GfxCommandListRecorder(GfxDevice* device, CommandListHandle cmdHandle);

	private:
		rhi::GfxDevice* m_device;
		platform::CommandListResource* m_platformResource;
		platform::GfxCommandListRecorder m_platformRecorder;

	};
}