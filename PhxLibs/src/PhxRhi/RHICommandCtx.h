#pragma once

#include <phx/rhi/RHITypes.h>
#include <RHIPlatformTypes.h>

namespace phx::rhi
{
	class CommandCtx
	{
	public:
		void RenderPassBegin()
		{
			m_platform.RenderPassBegin();
		}

		void RenderPassEnd()
		{
			m_platform.RenderPassEnd();
		}

		void SetViewports(phx::Span<Viewport> viewports)
		{
			m_platform.SetViewports(viewports);
		}

		void SetScissors(phx::Span<Rect> scissors)
		{
			m_platform.SetScissors(scissors);
		}

		void SetPipelineState(PipelineStateHandle handle)
		{
			m_platform.SetPipelineState(handle);
		}

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0)
		{
			m_platform.DrawIndexed(indexCount, instanceCount, startIndex, baseVertex, startInstance);
		}

		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0)
		{
			m_platform.Draw(vertexCount, instanceCount, startVertex, startInstance);
		}

		void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
		{
			m_platform.SetDynamicVertexBuffer(slot, numVertices, vertexSize, vertexBufferData);
		}

		void SetDynamicIndexBuffer(size_t numIndicies, Format indexFormat, const void* indexBufferData)
		{
			m_platform.SetDynamicIndexBuffer(numIndicies, indexFormat, indexBufferData);
		}

		void SetPushConstant(uint32_t rootParameterIndex, size_t sizeInBytes, const void* constants)
		{
			m_platform.SetPushConstant(rootParameterIndex, sizeInBytes, constants);
		}

		template<typename T>
		void SetPushConstant(uint32_t rootParameterIndex, T const& constants)
		{
			SetPushConstant(rootParameterIndex, sizeof(T), &constants);
		}

	public:
		PlatformCommandCtx& GetPlatform() { return m_platform; }

	private:
		PlatformCommandCtx m_platform;
	};
}