#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include "D3D12Common.h"
#include "CommandAllocatorPool.h"

namespace PhxEngine::RHI::D3D12
{
	class D3D12GraphicsDevice;
	class CommandQueue;
	class GpuDescriptorHeap;
	class UploadBuffer;
	class DynamicSuballocator;
	class D3D12ComputePipeline;

	class TimerQuery;
	struct TrackedResources
	{
		std::vector<std::shared_ptr<IRHIResource>> Resource;
		std::vector<Microsoft::WRL::ComPtr<IUnknown>> NativeResources;
		std::vector<std::shared_ptr<TimerQuery>> TimerQueries;
		std::vector<Core::Handle<Texture>> TextureHandles;
		bool IsEmpty()
		{
			return this->Resource.empty() &&
				this->NativeResources.empty() &&
				this->TimerQueries.empty() &&
				this->TextureHandles.empty();
		}

	};

	class DynamicSubAllocatorPool
	{
	public:
		DynamicSubAllocatorPool(
			GpuDescriptorHeap& gpuDescriptorHeap,
			uint32_t chunkSize);
		~DynamicSubAllocatorPool() = default;

		DynamicSuballocator* RequestAllocator(uint64_t completedFenceValue);
		void DiscardAllocator(uint64_t fence, DynamicSuballocator* allocator);

		inline size_t Size() { return this->m_allocatorPool.size(); }

	private:
		const uint32_t m_chunkSize;
		GpuDescriptorHeap& m_gpuDescriptorHeap;

		std::vector<std::unique_ptr<DynamicSuballocator>> m_allocatorPool;
		std::queue<std::pair<uint64_t, DynamicSuballocator*>> m_availableAllocators;
	};

	class D3D12CommandList final : public ICommandList
	{
	public:
		D3D12CommandList(CommandQueue* parentQueue);

		~D3D12CommandList();

		// -- Interface implementations ---
	public:
		void Open() override;
		void Close() override;

		// -- RayTrace Stuff		---]
		void RTBuildAccelerationStructure(RHI::RTAccelerationStructureHandle accelStructure) override;

		// -- RayTrace Stuff END	---
		ScopedMarker BeginScopedMarker(std::string name) override;
		void BeginMarker(std::string name) override;
		void EndMarker() override;
		GPUAllocation AllocateGpu(size_t bufferSize, size_t stride) override;

		void TransitionBarrier(TextureHandle texture, ResourceStates beforeState, ResourceStates afterState) override;
		void TransitionBarrier(BufferHandle buffer, ResourceStates beforeState, ResourceStates afterState) override;
		void TransitionBarriers(Core::Span<GpuBarrier> gpuBarriers) override;

		void BeginRenderPassBackBuffer() override;
		RenderPassHandle GetRenderPassBackBuffer() override;
		void BeginRenderPass(RenderPassHandle renderPass) override;
		void EndRenderPass() override;

		void ClearTextureFloat(TextureHandle texture, Color const& clearColour) override;
		void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) override;

		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0) override;
		void DrawIndexed(
			uint32_t indexCount,
			uint32_t instanceCount = 1,
			uint32_t startIndex = 0,
			int32_t baseVertex = 0,
			uint32_t startInstance = 0);

		void WriteBuffer(BufferHandle buffer, const void* data, size_t dataSize, uint64_t destOffsetBytes = 0) override;
		void CopyBuffer(BufferHandle dst, uint64_t dstOffset, BufferHandle src, uint64_t srcOffset, size_t sizeInBytes) override;
		void WriteTexture(TextureHandle texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData) override;
		void WriteTexture(TextureHandle texture, uint32_t arraySlice, uint32_t mipLevel, const void* data, size_t rowPitch, size_t depthPitch) override;
		void SetRenderTargets(std::vector<TextureHandle> const& renderTargets, TextureHandle depthStencil) override;

        void SetGraphicsPipeline(GraphicsPipelineHandle graphisPSO) override;
		void SetViewports(Viewport* viewports, size_t numViewports) override;
		void SetScissors(Rect* scissor, size_t numScissors) override;
		void BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants) override;
		void BindConstantBuffer(size_t rootParameterIndex, BufferHandle constantBuffer) override;
		void BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData) override;
		void BindVertexBuffer(uint32_t slot, BufferHandle vertexBuffer) override;
		void BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData) override;
		void BindIndexBuffer(BufferHandle indexBuffer) override;
		void BindDynamicIndexBuffer(size_t numIndicies, RHIFormat indexFormat, const void* indexBufferData) override;
        void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData) override;
        void BindStructuredBuffer(size_t rootParameterIndex, BufferHandle buffer) override;
		void BindResourceTable(size_t rootParameterIndex) override;
		void BindSamplerTable(size_t rootParameterIndex) override;
		void BindDynamicDescriptorTable(size_t rootParameterIndex, Core::Span<TextureHandle> textures) override;
		void BindDynamicUavDescriptorTable(size_t rootParameterIndex, std::vector<TextureHandle> const& textures) override;

		// -- Comptute Stuff ---
		void SetComputeState(ComputePipelineHandle state);
		void Dispatch(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1);
		void DispatchIndirect(uint32_t offsetBytes);

		// -- Query Stuff ---
		void BeginTimerQuery(TimerQueryHandle query);
		void EndTimerQuery(TimerQueryHandle query);

	public:
		std::shared_ptr<TrackedResources> Executed(uint64_t fenceValue);
		ID3D12CommandList* GetD3D12CommandList() { return this->m_d3d12CommandList.Get(); }

		void SetDescritporHeaps(std::array<GpuDescriptorHeap*, 2> const& shaderHeaps);

	private:
		void TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	private:
		const uint32_t DynamicChunkSizeSrvUavCbv = 256;
		CommandQueue* m_parentQueue;
		D3D12GraphicsDevice& m_graphicsDevice;
		D3D12ComputePipeline* m_activeComputePipeline = nullptr;

		std::unique_ptr<UploadBuffer> m_uploadBuffer;

		std::shared_ptr<TrackedResources> m_trackedData;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_activeD3D12CommandAllocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_d3d12CommandList;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_d3d12CommandList6;

		// Temp Memory pool used for GPU Barriers
		std::vector<D3D12_RESOURCE_BARRIER> m_barrierMemoryPool;

		// Dynamic Descriptor Heap
		DynamicSubAllocatorPool m_dynamicSubAllocatorPool;
		DynamicSuballocator* m_activeDynamicSubAllocator;

		RHI::RenderPassHandle m_activeRenderTarget;
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_buildGeometries;
	};
}

