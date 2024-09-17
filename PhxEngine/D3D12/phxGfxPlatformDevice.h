#pragma once

#include "phxGfxCommonResources.h"
#include "phxGfxPlatformResources.h"
#include "phxGfxDescriptorHeap.h"
#include "phxGfxHandlePool.h"
#include "phxSpan.h"
#include "d3d12ma/D3D12MemAlloc.h"

#include <mutex>
#include <queue>

// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2


namespace phx::gfx
{
	constexpr size_t kNumCommandListPerFrame = 32;
	constexpr size_t kResourcePoolSize = 100000; // 1 KB of handles
	constexpr size_t kNumConcurrentRenderTargets = 8;

	enum class DescriptorHeapTypes : uint8_t
	{
		CBV_SRV_UAV,
		Sampler,
		RTV,
		DSV,
		Count,
	};

	struct D3D12CommandQueue final
	{
		D3D12_COMMAND_LIST_TYPE m_type;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_platformQueue;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_platformFence;

		std::mutex m_commandListMutx;

		uint64_t m_nextFenceValue = 0;
		uint64_t m_lastCompletedFenceValue = 0;

		std::mutex m_fenceMutex;
		std::mutex m_eventMutex;
		HANDLE m_fenceEvent;

		std::vector<ID3D12CommandList*> m_pendingCmdLists;
		std::vector<ID3D12CommandAllocator*> m_pendingAllocators;

		void Initialize(ID3D12Device* nativeDevice, D3D12_COMMAND_LIST_TYPE type);
		void Finailize();

		D3D12_COMMAND_LIST_TYPE GetType() const { return this->m_type; }

		void EnqueueCommandList(ID3D12CommandList* cmdList, ID3D12CommandAllocator* allocator)
		{
			this->m_pendingCmdLists.push_back(cmdList);
			this->m_pendingAllocators.push_back(allocator);
		}

		uint64_t Submit();

		uint64_t GetLastCompletedFence();
		uint64_t ExecuteCommandLists(Span<ID3D12CommandList*> commandLists);
		ID3D12CommandAllocator* RequestAllocator();
		void DiscardAllocators(uint64_t fence, Span<ID3D12CommandAllocator*> allocators);
		void DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator);

		uint64_t IncrementFence();
		uint64_t GetNextFenceValue() { return this->m_nextFenceValue + 1; }
		bool IsFenceComplete(uint64_t fenceValue);
		void WaitForFence(uint64_t fenceValue);
		void WaitForIdle() { this->WaitForFence(this->IncrementFence()); }
		class CommandAllocatorPool
		{
		public:
			void Initialize(ID3D12Device* nativeDevice, D3D12_COMMAND_LIST_TYPE type);
			ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
			void DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator);

		private:
			std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_allocatorPool;
			std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_availableAllocators;
			std::mutex m_allocatonMutex;
			ID3D12Device* m_nativeDevice;
			D3D12_COMMAND_LIST_TYPE m_type;
		} m_allocatorPool;
	};

	using CommandQueue = D3D12CommandQueue;

	struct D3D12GpuAdapter final
	{
		std::wstring Name;
		size_t DedicatedSystemMemory = 0;
		size_t DedicatedVideoMemory = 0;
		size_t SharedSystemMemory = 0;
		D3D12DeviceBasicInfo BasicDeviceInfo;
		DXGI_ADAPTER_DESC PlatformDesc;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> PlatformAdapter;

		static HRESULT EnumAdapters(uint32_t adapterIndex, IDXGIFactory6* factory6, IDXGIAdapter1** outAdapter);
	};

	using PlatformGpuAdpater = D3D12GpuAdapter;

	struct D3D12DeviceBasicInfo final
	{
		uint32_t NumDeviceNodes;
	};

	class D3D12BindlessDescriptorTable : NonCopyable
	{
	public:

		void Initialize(DescriptorHeapAllocation&& allocation)
		{
			this->m_allocation = std::move(allocation);
		}

	public:
		DescriptorIndex Allocate()
		{
			return this->m_descriptorIndexPool.Allocate();
		}

		void Free(DescriptorIndex index)
		{
			this->m_descriptorIndexPool.Release(index);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(DescriptorIndex index) const { return this->m_allocation.GetCpuHandle(index); }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(DescriptorIndex index) const { return this->m_allocation.GetGpuHandle(index); }

	private:
		struct DescriptorIndexPool
		{
			// Removes the first element from the free list and returns its index
			UINT Allocate()
			{
				std::scoped_lock Guard(this->IndexMutex);

				UINT NewIndex;
				if (!IndexQueue.empty())
				{
					NewIndex = IndexQueue.front();
					IndexQueue.pop();
				}
				else
				{
					NewIndex = Index++;
				}
				return NewIndex;
			}

			void Release(UINT index)
			{
				std::scoped_lock Guard(this->IndexMutex);
				IndexQueue.push(index);
			}

			std::mutex IndexMutex;
			std::queue<DescriptorIndex> IndexQueue;
			UINT Index = 0;
		};

	private:
		DescriptorHeapAllocation m_allocation;
		DescriptorIndexPool m_descriptorIndexPool;

	};

	class D3D12Device final
	{
	public:
		D3D12Device();
		~D3D12Device();

		inline static D3D12Device* Ptr = nullptr;

		// -- Interface Functions ---
		// -- Frame Functions ---
	public:
		void Initialize();
		void Finalize();

		// -- Submits Command lists and presents ---
		void SubmitFrame();
		void WaitForIdle();

		bool IsDevicedRemoved();

		// -- Resouce Functions ---
	public:
		SwapChainHandle CreateSwapChain(SwapChainDesc const& swapchainDesc, void* windowHandle);

		template<typename T>
		CommandSignatureHandle CreateCommandSignature(CommandSignatureDesc const& desc)
		{
			static_assert(sizeof(T) % sizeof(uint32_t) == 0);
			return this->CreateCommandSignature(desc, sizeof(T));
		}
		CommandSignatureHandle CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride);
		ShaderHandle CreateShader(ShaderDesc const& desc, Span<uint8_t> shaderByteCode);
		InputLayoutHandle CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount);
		GfxPipelineHandle CreateGfxPipeline(GfxPipelineDesc const& desc);
		ComputePipelineHandle CreateComputePipeline(ComputePipelineDesc const& desc);
		MeshPipelineHandle CreateMeshPipeline(MeshPipelineDesc const& desc);
		TextureHandle CreateTexture(TextureDesc const& desc);
		RenderPassHandle CreateRenderPass(RenderPassDesc const& desc);
		BufferHandle CreateBuffer(BufferDesc const& desc);
		int CreateSubresource(TextureHandle texture, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip = 0, uint32_t mpCount = ~0);
		int CreateSubresource(BufferHandle buffer, SubresouceType subresourceType, size_t offset, size_t size = ~0u);
		RTAccelerationStructureHandle CreateRTAccelerationStructure(RTAccelerationStructureDesc const& desc);
		TimerQueryHandle CreateTimerQuery();


		void DeleteCommandSignature(CommandSignatureHandle handle);
		void DeleteGfxPipeline(GfxPipelineHandle handle);


		void DeleteMeshPipeline(MeshPipelineHandle handle);

		const TextureDesc& GetTextureDesc(TextureHandle handle);
		DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type, int subResource = -1);
		void DeleteTexture(TextureHandle handle);

		void GetRenderPassFormats(RenderPassHandle handle, std::vector<gfx::Format>& outRtvFormats, gfx::Format& depthFormat);
		RenderPassDesc GetRenderPassDesc(RenderPassHandle);
		void DeleteRenderPass(RenderPassHandle handle);

		// -- TODO: End Remove

		const BufferDesc& GetBufferDesc(BufferHandle handle);
		DescriptorIndex GetDescriptorIndex(BufferHandle handle, SubresouceType type, int subResource = -1);

		template<typename T>
		T* GetBufferMappedData(BufferHandle handle)
		{
			return static_cast<T*>(this->GetBufferMappedData(handle));
		};

		void* GetBufferMappedData(BufferHandle handle);
		uint32_t GetBufferMappedDataSizeInBytes(BufferHandle handle);
		void DeleteBuffer(BufferHandle handle);

		// -- Ray Tracing ---
		size_t GetRTTopLevelAccelerationStructureInstanceSize();
		void WriteRTTopLevelAccelerationStructureInstance(RTAccelerationStructureDesc::TopLevelDesc::Instance const& instance, void* dest);
		const RTAccelerationStructureDesc& GetRTAccelerationStructureDesc(RTAccelerationStructureHandle handle);
		void DeleteRtAccelerationStructure(RTAccelerationStructureHandle handle);
		DescriptorIndex GetDescriptorIndex(RTAccelerationStructureHandle handle);

		// -- Query Stuff ---
#if false
		void DeleteTimerQuery(TimerQueryHandle query);
		bool PollTimerQuery(TimerQueryHandle query);
		TimeStep GetTimerQueryTime(TimerQueryHandle query);
		void ResetTimerQuery(TimerQueryHandle query);
#endif

		// -- Utility ---
	public:
		TextureHandle GetBackBuffer(SwapChainHandle handle);
		size_t GetNumBindlessDescriptors() const { return NUM_BINDLESS_RESOURCES; }

		ShaderModel GetMinShaderModel() const { return this->m_minShaderModel; };
		ShaderType GetShaderType() const { return ShaderType::HLSL6; };
		GraphicsAPI GetApi() const { return GraphicsAPI::DX12; }

		void BeginCapture(std::wstring const& filename);
		void EndCapture(bool discard = false);

		bool CheckCapability(DeviceCapability deviceCapability);

		float GetAvgFrameTime();

		// float GetAvgFrameTime() { return this->m_frameStats.GetAvg(); };
		uint64_t GetUavCounterPlacementAlignment() { return D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT; }
		// -- Command list Functions ---
		MemoryUsage GetMemoryUsage() const
		{
			MemoryUsage retval;
			D3D12MA::Budget budget;
			this->m_d3d12MemAllocator->GetBudget(&budget, nullptr);
			retval.Budget = budget.BudgetBytes;
			retval.Usage = budget.UsageBytes;
			return retval;
		}

#if false
		// These are not thread Safe
	public:
		// TODO: Change to a new pattern so we don't require a command list stored on an object. Instread, request from a pool of objects
		CommandListHandle BeginCommandList(CommandQueueType queueType = CommandQueueType::Graphics);
		void WaitCommandList(CommandListHandle cmd, CommandListHandle WaitOn);

		// -- Ray Trace stuff       ---
		void RTBuildAccelerationStructure(gfx::RTAccelerationStructureHandle accelStructure, CommandListHandle cmd);

		// -- Ray Trace Stuff END   ---
		void BeginMarker(std::string_view name, CommandListHandle cmd);
		void EndMarker(CommandListHandle cmd);
		GPUAllocation AllocateGpu(size_t bufferSize, size_t stride, CommandListHandle cmd);

		void TransitionBarrier(TextureHandle texture, ResourceStates beforeState, ResourceStates afterState, CommandListHandle cmd);
		void TransitionBarrier(BufferHandle buffer, ResourceStates beforeState, ResourceStates afterState, CommandListHandle cmd);
		void TransitionBarriers(Span<GpuBarrier> gpuBarriers, CommandListHandle cmd);
		void ClearTextureFloat(TextureHandle texture, Color const& clearColour, CommandListHandle cmd);
		void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil, CommandListHandle cmd);

		void Draw(DrawArgs const& args, CommandListHandle cmd);
		void DrawIndexed(DrawArgs const& args, CommandListHandle cmd);

		void ExecuteIndirect(gfx::CommandSignatureHandle commandSignature, gfx::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);
		void ExecuteIndirect(gfx::CommandSignatureHandle commandSignature, gfx::BufferHandle args, size_t argsOffsetInBytes, gfx::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);

		void DrawIndirect(gfx::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);
		void DrawIndirect(gfx::BufferHandle args, size_t argsOffsetInBytes, gfx::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);

		void DrawIndexedIndirect(gfx::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);
		void DrawIndexedIndirect(gfx::BufferHandle args, size_t argsOffsetInBytes, gfx::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);

		template<typename T>
		void WriteBuffer(BufferHandle buffer, std::vector<T> const& data, uint64_t destOffsetBytes, CommandListHandle cmd)
		{
			this->WriteBuffer(buffer, data.data(), sizeof(T) * data.size(), destOffsetBytes, cmd);
		}

		template<typename T>
		void WriteBuffer(BufferHandle buffer, T const& data, uint64_t destOffsetBytes, CommandListHandle cmd)
		{
			this->WriteBuffer(buffer, &data, sizeof(T), destOffsetBytes, cmd);
		}

		template<typename T>
		void WriteBuffer(BufferHandle buffer, Span<T> data, uint64_t destOffsetBytes, CommandListHandle cmd)
		{
			this->WriteBuffer(buffer, data.begin(), sizeof(T) * data.Size(), destOffsetBytes, cmd);
		}

		void WriteBuffer(BufferHandle buffer, const void* Data, size_t dataSize, uint64_t destOffsetBytes, CommandListHandle cmd);

		void CopyBuffer(BufferHandle dst, uint64_t dstOffset, BufferHandle src, uint64_t srcOffset, size_t sizeInBytes, CommandListHandle cmd);

		void WriteTexture(TextureHandle texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData, CommandListHandle cmd);
		void WriteTexture(TextureHandle texture, uint32_t arraySlice, uint32_t mipLevel, const void* Data, size_t rowPitch, size_t depthPitch, CommandListHandle cmd);

		void SetGfxPipeline(GfxPipelineHandle gfxPipeline, CommandListHandle cmd);
		void SetViewports(Viewport* viewports, size_t numViewports, CommandListHandle cmd);
		void SetScissors(Rect* scissor, size_t numScissors, CommandListHandle cmd);
		void SetRenderTargets(Span<TextureHandle> renderTargets, TextureHandle depthStenc, CommandListHandle cmd);

		// -- Comptute Stuff ---
		void SetComputeState(ComputePipelineHandle state, CommandListHandle cmd);
		void Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ, CommandListHandle cmd);
		void DispatchIndirect(gfx::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);
		void DispatchIndirect(gfx::BufferHandle args, uint32_t argsOffsetInBytes, gfx::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);

		// -- Mesh Stuff ---
		void SetMeshPipeline(MeshPipelineHandle meshPipeline, CommandListHandle cmd);
		void DispatchMesh(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ, CommandListHandle cmd);
		void DispatchMeshIndirect(gfx::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);
		void DispatchMeshIndirect(gfx::BufferHandle args, uint32_t argsOffsetInBytes, gfx::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);

		void BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants, CommandListHandle cmd);
		template<typename T>
		void BindPushConstant(uint32_t rootParameterIndex, const T& constants, CommandListHandle cmd)
		{
			static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
			this->BindPushConstant(rootParameterIndex, sizeof(T), &constants, cmd);
		}

		void BindConstantBuffer(size_t rootParameterIndex, BufferHandle constantBuffer, CommandListHandle cmd);
		void BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData, CommandListHandle cmd);
		template<typename T>
		void BindDynamicConstantBuffer(size_t rootParameterIndex, T const& bufferData, CommandListHandle cmd)
		{
			this->BindDynamicConstantBuffer(rootParameterIndex, sizeof(T), &bufferData, cmd);
		}

		void BindVertexBuffer(uint32_t slot, BufferHandle vertexBuffer, CommandListHandle cmd);

		/**
		 * Set dynamic vertex buffer data to the rendering pipeline.
		 */
		void BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData, CommandListHandle cmd);

		template<typename T>
		void BindDynamicVertexBuffer(uint32_t slot, const std::vector<T>& vertexBufferData, CommandListHandle cmd)
		{
			this->BindDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data(), cmd);
		}

		void BindIndexBuffer(BufferHandle bufferHandle, CommandListHandle cmd);

		/**
		 * Bind dynamic index buffer data to the rendering pipeline.
		 */
		void BindDynamicIndexBuffer(size_t numIndicies, gfx::Format indexFormat, const void* indexBufferData, CommandListHandle cmd);

		template<typename T>
		void BindDynamicIndexBuffer(const std::vector<T>& indexBufferData, CommandListHandle cmd)
		{
			staticassert(sizeof(T) == 2 || sizeof(T) == 4);

			gfx::Format indexFormat = (sizeof(T) == 2) ? gfx::Format::R16_UINT : gfx::Format::R32_UINT;
			this->BindDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.Data(), cmd);
		}

		/**
		 * Set dynamic structured buffer contents.
		 */
		void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData, CommandListHandle cmd);

		template<typename T>
		void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, std::vector<T> const& bufferData, CommandListHandle cmd)
		{
			this->BindDynamicStructuredBuffer(rootParameterIndex, bufferData.size(), sizeof(T), bufferData.Data(), cmd);
		}

		void BindStructuredBuffer(size_t rootParameterIndex, BufferHandle buffer, CommandListHandle cmd);

		void BindDynamicDescriptorTable(size_t rootParameterIndex, Span<TextureHandle> textures, CommandListHandle cmd);
		void BindDynamicUavDescriptorTable(
			size_t rootParameterIndex,
			Span<BufferHandle> buffers,
			CommandListHandle cmd)
		{
			this->BindDynamicUavDescriptorTable(rootParameterIndex, buffers, {}, cmd);
		}
		void BindDynamicUavDescriptorTable(
			size_t rootParameterIndex,
			Span<TextureHandle> textures,
			CommandListHandle cmd)
		{
			this->BindDynamicUavDescriptorTable(rootParameterIndex, {}, textures, cmd);
		}
		void BindDynamicUavDescriptorTable(
			size_t rootParameterIndex,
			Span<BufferHandle> buffers,
			Span<TextureHandle> textures,
			CommandListHandle cmd);

		void BindResourceTable(size_t rootParameterIndex, CommandListHandle cmd);
		void BindSamplerTable(size_t rootParameterIndex, CommandListHandle cmd);

		void BeginTimerQuery(TimerQueryHandle query, CommandListHandle cmd);
		void EndTimerQuery(TimerQueryHandle query, CommandListHandle cmd);
#endif
		// -- Dx12 Specific functions ---
	public:
		void DeleteD3DResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource);

		const D3D12SwapChain& GetSwapChain() const { return this->m_swapChain; }
		TextureHandle CreateRenderTarget(TextureDesc const& desc, Microsoft::WRL::ComPtr<ID3D12Resource> d3d12TextureResource);
		TextureHandle CreateTexture(TextureDesc const& desc, Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource);

		int CreateShaderResourceView(BufferHandle buffer, size_t offset, size_t size);
		int CreateUnorderedAccessView(BufferHandle buffer, size_t offset, size_t size);

		int CreateShaderResourceView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
		int CreateRenderTargetView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
		int CreateDepthStencilView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
		int CreateUnorderedAccessView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);

	public:
		void CreateBackBuffers(D3D12SwapChain* viewport);
		void RunGarbageCollection(uint64_t completedFrame);

		// -- Getters ---
	public:
		Microsoft::WRL::ComPtr<ID3D12Device> GetD3D12Device() { return this->m_rootDevice; }
		Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device2() { return this->m_rootDevice2; }
		Microsoft::WRL::ComPtr<ID3D12Device5> GetD3D12Device5() { return this->m_rootDevice5; }

		Microsoft::WRL::ComPtr<IDXGIFactory6> GetDxgiFactory() { return this->m_factory; }
		Microsoft::WRL::ComPtr<IDXGIAdapter> GetDxgiAdapter() { return this->m_gpuAdapter.PlatformAdapter; }

	public:
		D3D12CommandQueue& GetGfxQueue() { return this->GetQueue(CommandQueueType::Graphics); }
		D3D12CommandQueue& GetComputeQueue() { return this->GetQueue(CommandQueueType::Compute); }
		D3D12CommandQueue& GetCopyQueue() { return this->GetQueue(CommandQueueType::Copy); }

		D3D12CommandQueue& GetQueue(CommandQueueType type) { return this->m_commandQueues[(int)type]; }

		// -- Command Signatures ---
		ID3D12CommandSignature* GetDispatchIndirectCommandSignature() const { return this->m_dispatchIndirectCommandSignature.Get(); }
		ID3D12CommandSignature* GetDrawInstancedIndirectCommandSignature() const { return this->m_drawInstancedIndirectCommandSignature.Get(); }
		ID3D12CommandSignature* GetDrawIndexedInstancedIndirectCommandSignature() const { return this->m_drawIndexedInstancedIndirectCommandSignature.Get(); }
		ID3D12CommandSignature* GetDispatchMeshIndirectCommandSignature() const { return this->m_dispatchMeshIndirectCommandSignature.Get(); }

		CpuDescriptorHeap& GetResourceCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV]; }
		CpuDescriptorHeap& GetSamplerCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler]; }
		CpuDescriptorHeap& GetRtvCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::RTV]; }
		CpuDescriptorHeap& GetDsvCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::DSV]; }

		GpuDescriptorHeap& GetResourceGpuHeap() { return this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV]; }
		GpuDescriptorHeap& GetSamplerGpuHeap() { return this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler]; }

#if false
		ID3D12QueryHeap* GetQueryHeap() { return this->m_gpuTimestampQueryHeap.Get(); }
		BufferHandle GetTimestampQueryBuffer() { return this->m_timestampQueryBuffer; }
#endif
		const D3D12BindlessDescriptorTable& GetBindlessTable() const { return this->m_bindlessResourceDescriptorTable; }

		// Maybe better encapulate this.
		HandlePool<D3D12SwapChain, SwapChain>& GetSwapChainPool() { return this->m_swapChainPool; }
		HandlePool<D3D12Texture, Texture>& GetTexturePool() { return this->m_texturePool; };
		HandlePool<D3D12Buffer, Buffer>& GetBufferPool() { return this->m_bufferPool; };
		HandlePool<D3D12RenderPass, RenderPass>& GetRenderPassPool() { return this->m_renderPassPool; };
		HandlePool<D3D12GraphicsPipeline, GfxPipeline>& GetGraphicsPipelinePool() { return this->m_gfxPipelinePool; }
		HandlePool<D3D12ComputePipeline, ComputePipeline>& GetComputePipelinePool() { return this->m_computePipelinePool; }
		HandlePool<D3D12MeshPipeline, MeshPipeline>& GetMeshPipelinePool() { return this->m_meshPipelinePool; }
		HandlePool<D3D12RTAccelerationStructure, RTAccelerationStructure>& GetRTAccelerationStructurePool() { return this->m_rtAccelerationStructurePool; }
		HandlePool<D3D12CommandSignature, CommandSignature>& GetCommandSignaturePool() { return this->m_commandSignaturePool; }
		// HandlePool<D3D12TimerQuery, TimerQuery>& GetTimerQueryPool() { return this->m_timerQueryPool; }

	private:
		bool IsHdrSwapchainSupported();

	private:
		void CreateBufferInternal(BufferDesc const& desc, D3D12Buffer& outBuffer);

		void CreateGpuTimestampQueryHeap(uint32_t queryCount);

		// -- Dx12 API creation ---
	private:
		void InitializeResourcePools();

		void InitializeD3D12NativeResources(IDXGIAdapter* gpuAdapter);

		void FindAdapter(Microsoft::WRL::ComPtr<IDXGIFactory6> factory, D3D12GpuAdapter& outAdapter);

		// -- Pipeline state conversion --- 
	private:
		void TranslateBlendState(BlendRenderState const& inState, D3D12_BLEND_DESC& outState);
		void TranslateDepthStencilState(DepthStencilRenderState const& inState, D3D12_DEPTH_STENCIL_DESC& outState);
		void TranslateRasterState(RasterRenderState const& inState, D3D12_RASTERIZER_DESC& outState);

	private:
		const uint32_t kTimestampQueryHeapSize = 1024;

		Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
		Microsoft::WRL::ComPtr<ID3D12Device> m_rootDevice;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_rootDevice2;
		Microsoft::WRL::ComPtr<ID3D12Device5> m_rootDevice5;

		// -- Command Signatures ---
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_dispatchIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_drawInstancedIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_drawIndexedInstancedIndirectCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_dispatchMeshIndirectCommandSignature;

		Microsoft::WRL::ComPtr<D3D12MA::Allocator> m_d3d12MemAllocator;

		// std::shared_ptr<IDxcUtils> dxcUtils;
		D3D12GpuAdapter m_gpuAdapter;

		D3D12_FEATURE_DATA_ROOT_SIGNATURE FeatureDataRootSignature = {};
		D3D12_FEATURE_DATA_SHADER_MODEL   FeatureDataShaderModel = {};
		ShaderModel m_minShaderModel = ShaderModel::SM_6_0;

		bool IsUnderGraphicsDebugger = false;
		gfx::DeviceCapability m_capabilities;

		D3D12SwapChain m_swapChain;

		// -- Resouce Pool ---
		HandlePool<D3D12SwapChain, SwapChain> m_swapChainPool;
		HandlePool<D3D12Texture, Texture> m_texturePool;
		HandlePool<D3D12CommandSignature, CommandSignature> m_commandSignaturePool;
		HandlePool<D3D12Shader, gfx::Shader> m_shaderPool;
		HandlePool<D3D12InputLayout, InputLayout> m_inputLayoutPool;
		HandlePool<D3D12Buffer, Buffer> m_bufferPool;
		HandlePool<D3D12RenderPass, RenderPass> m_renderPassPool;
		HandlePool<D3D12RTAccelerationStructure, RTAccelerationStructure> m_rtAccelerationStructurePool;
		HandlePool<D3D12GraphicsPipeline, GfxPipeline> m_gfxPipelinePool;
		HandlePool<D3D12ComputePipeline, ComputePipeline> m_computePipelinePool;
		HandlePool<D3D12MeshPipeline, MeshPipeline> m_meshPipelinePool;
		// HandlePool<D3D12TimerQuery, TimerQuery> m_timerQueryPool;

		// -- Command Queues ---
		std::array<D3D12CommandQueue, (int)CommandQueueType::Count> m_commandQueues;

		// -- Command lists ---
		uint32_t m_activeCmdCount;
		// std::array<D3D12CommandList, 32> m_commandListPool;

		// -- Descriptor Heaps ---
		std::array<CpuDescriptorHeap, (int)DescriptorHeapTypes::Count> m_cpuDescriptorHeaps;
		std::array<GpuDescriptorHeap, 2> m_gpuDescriptorHeaps;

		// -- Query Heaps ---
#if false
		Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_gpuTimestampQueryHeap;
		BufferHandle m_timestampQueryBuffer;
		BitSetAllocator m_timerQueryIndexPool;
#endif

		D3D12BindlessDescriptorTable m_bindlessResourceDescriptorTable;

		// -- Frame Frences --
		std::vector<EnumArray<CommandQueueType, Microsoft::WRL::ComPtr<ID3D12Fence>>> m_frameFences;
		uint64_t m_frameCount = 1;

		struct DeleteItem
		{
			uint64_t Frame;
			std::function<void()> DeleteFn;
		};
		std::deque<DeleteItem> m_deleteQueue;

#ifdef ENABLE_PIX_CAPUTRE
		HMODULE m_pixCaptureModule;
#endif
	};

	using PlatformDevice = D3D12Device;
}