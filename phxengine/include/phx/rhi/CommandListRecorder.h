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
			m_platformRecorder.Close(m_platformResource);
		}

		void RenderPassBegin(SwapChainHandle handle)
		{
			auto* binding = m_device->GetSwapChainPool().Get<platform::SwapChainBindings>(handle);
			m_platformRecorder.RenderPassBegin(m_platformResource, binding);
		}

		void RenderPassEnd()
		{
			m_platformRecorder.RenderPassEnd(m_platformResource);
		}

		void SetViewports(phx::Span<Viewport> viewports)
		{
			m_platformRecorder.RenderPassEnd(m_platformResource);
		}

		void SetScissors(phx::Span<Rect> scissors)
		{
			m_platformRecorder.RenderPassEnd(m_platformResource);

		}

		void SetPipelineState(PipelineStateHandle handle)
		{
			m_platformRecorder.RenderPassEnd(m_platformResource);
		}

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0)
		{
			m_platformRecorder.RenderPassEnd(m_platformResource);

		}

		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0)
		{
			m_platformRecorder.RenderPassEnd(m_platformResource);
		}

		void SetDynamicVertexBuffer(GpuBufferHandle tempBuffer, size_t offset, uint32_t slot, size_t numVertices, size_t vertexSize)
		{
			m_platformRecorder.RenderPassEnd(m_platformResource);
		}

		void SetDynamicIndexBuffer(GpuBufferHandle tempBuffer, size_t offset, size_t numIndicies, Format indexFormat)
		{
			m_platformRecorder.RenderPassEnd(m_platformResource);
		}

		void SetPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants)
		{
			m_platformRecorder.RenderPassEnd(m_platformResource);
		}

		template<typename T>
		void SetPushConstant(uint32_t rootParameterIndex, T const& constants)
		{
			SetPushConstant(rootParameterIndex, sizeof(T), &constants);
		}

	protected:
		GfxCommandListRecorder(GfxDevice* device, CommandListHandle cmdHandle);

	private:
		GfxDevice* m_device;
		platform::CommandListResource* m_platformResource;
		platform::GfxCommandListRecorder m_platformRecorder;

	};
}