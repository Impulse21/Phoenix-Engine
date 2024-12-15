#pragma once

#include "phx/core/Memory.h"
#include "RHITypes.h"
#include "PlatformTypes.h"
#include "GfxDevice.h"
#include "TempMemoryBlockAllocator.h"

namespace phx::rhi
{
	class GfxDevice;

	// One per thread - not thread safe

	struct DynamicAllocation
	{
		rhi::GpuBufferHandle BufferHandle;
		size_t Offset;
		uint8_t* Data;

		void Set(const void* src, size_t size)
		{
			std::memcpy(Data, src, size);
		}
	};

	struct DynamicAllocator
	{
		DynamicAllocator(TempMemoryBlockAllocator& blockAllocator)
			: BlockAllocator(blockAllocator)
			, ByteOffset(0) 
		{
		}

		DynamicAllocation Allocate(uint32_t byteSize, uint32_t alignment)
		{
			if (!Block.BufferHandle.IsValid())
			{
				Block = BlockAllocator.GetNextMemoryBlock();
			}

			uint32_t offset = AlignUp(ByteOffset, alignment);
			ByteOffset = offset + byteSize;

			if (ByteOffset > BlockAllocator.GetBlockSize())
			{
				Block = BlockAllocator.GetNextMemoryBlock();
				offset = 0;
				ByteOffset = byteSize;
			}

			return DynamicAllocation{
				.BufferHandle = Block.BufferHandle,
				.Offset = offset + Block.Offset,
				.Data = Block.Data + offset
			};
		}

		DynamicMemoryBlock Block = {};
		uint32_t ByteOffset = 0;
		TempMemoryBlockAllocator& BlockAllocator;
	};


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

		void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
		{
			DynamicAllocation alloc = m_dynamicAllocator.Allocate(vertexSize, 16);
			alloc.Set(vertexBufferData, numVertices * vertexSize);

			auto* platformBindings = m_device->GetGpuBufferPool().Get<platform::GpuBufferBindings>(alloc.BufferHandle);
			m_platformRecorder.SetDynamicVertexBuffer(platformBindings, alloc.Offset, slot, numVertices, vertexSize);
		}

		void SetDynamicIndexBuffer(size_t numIndicies, Format indexFormat, const void* indexBufferData)
		{
			const size_t indexStrideInBytes = indexFormat == Format::R16_UINT ? 2 : 4;
			const size_t indexSizeInBytes = numIndicies * indexStrideInBytes;

			DynamicAllocation alloc = m_dynamicAllocator.Allocate(indexSizeInBytes, 16);
			alloc.Set(indexBufferData, indexSizeInBytes);

			auto* platformBindings = m_device->GetGpuBufferPool().Get<platform::GpuBufferBindings>(alloc.BufferHandle);
			m_platformRecorder.SetDynamicIndexBuffer(platformBindings, alloc.Offset, numIndicies, indexFormat);
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
		DynamicAllocator m_dynamicAllocator;
		platform::CommandListResource* m_platformResource;
		platform::GfxCommandListRecorder m_platformRecorder;

	};
}