#pragma once

#include <mutex>

#include "RHI/phxRHI.h"
#include "d3d12ma/D3D12MemAlloc.h"
#include "Core/phxHandlePool.h"
#include "Core/phxEnumArray.h"

namespace phx::rhi
{
	struct DescriptorAllocationHanlder;
	struct D3D12CommandQueue;
	class CommandContextManager;
	struct D3D12Texture;

	struct DxgiFormatMapping
	{
		rhi::Format AbstractFormat;
		DXGI_FORMAT ResourceFormat;
		DXGI_FORMAT SrvFormat;
		DXGI_FORMAT RtvFormat;
	};

	const DxgiFormatMapping& GetDxgiFormatMapping(rhi::Format abstractFormat);

	const DxgiFormatMapping& GetDxgiFormatMapping(rhi::Format abstractFormat);

	inline D3D12_RESOURCE_STATES ConvertResourceStates(ResourceStates stateBits)
	{
		if (stateBits == ResourceStates::Common)
			return D3D12_RESOURCE_STATE_COMMON;

		D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON; // also 0

		if (EnumHasAnyFlags(stateBits, ResourceStates::ConstantBuffer)) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (EnumHasAnyFlags(stateBits, ResourceStates::VertexBuffer)) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (EnumHasAnyFlags(stateBits, ResourceStates::IndexGpuBuffer)) result |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if (EnumHasAnyFlags(stateBits, ResourceStates::IndirectArgument)) result |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		if (EnumHasAnyFlags(stateBits, ResourceStates::ShaderResource)) result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::UnorderedAccess)) result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if (EnumHasAnyFlags(stateBits, ResourceStates::RenderTarget)) result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		if (EnumHasAnyFlags(stateBits, ResourceStates::DepthWrite)) result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::DepthRead)) result |= D3D12_RESOURCE_STATE_DEPTH_READ;
		if (EnumHasAnyFlags(stateBits, ResourceStates::StreamOut)) result |= D3D12_RESOURCE_STATE_STREAM_OUT;
		if (EnumHasAnyFlags(stateBits, ResourceStates::CopyDest)) result |= D3D12_RESOURCE_STATE_COPY_DEST;
		if (EnumHasAnyFlags(stateBits, ResourceStates::CopySource)) result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::ResolveDest)) result |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
		if (EnumHasAnyFlags(stateBits, ResourceStates::ResolveSource)) result |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::Present)) result |= D3D12_RESOURCE_STATE_PRESENT;
		if (EnumHasAnyFlags(stateBits, ResourceStates::AccelStructRead)) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::AccelStructWrite)) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::AccelStructBuildInput)) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::AccelStructBuildBlas)) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::ShadingRateSurface)) result |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::GenericRead)) result |= D3D12_RESOURCE_STATE_GENERIC_READ;
		if (EnumHasAnyFlags(stateBits, ResourceStates::ShaderResourceNonPixel)) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

		return result;
	}

	class D3D12GfxDevice final : public GfxDevice
	{
	public:
		static inline GfxDevice* Ptr = nullptr;
		D3D12GfxDevice(Config const& config);
		~D3D12GfxDevice();
		
		// -- Public Interface ---
	public:
		void ResizeSwapchain(Rect const& size) override;

		void SubmitFrame() override;
		void WaitForIdle() override;

		TextureHandle GetBackBuffer() override;

		RenderContext& BeginContext() override;

		// -- Getters ---
	public:
		Microsoft::WRL::ComPtr<ID3D12Device>&  GetD3D12Device()  { return this->m_d3dDevice; }
		Microsoft::WRL::ComPtr<ID3D12Device2>& GetD3D12Device2() { return this->m_d3dDevice2; }
		Microsoft::WRL::ComPtr<ID3D12Device5>& GetD3D12Device5() { return this->m_d3dDevice5; }

		D3D12CommandQueue* GetQueueGfx() { return this->m_queues[rhi::CommandQueueType::Graphics].get(); }
		D3D12CommandQueue* GetQueueCompute() { return this->m_queues[rhi::CommandQueueType::Compute].get(); }
		D3D12CommandQueue* GetQueueCopy() { return this->m_queues[rhi::CommandQueueType::Copy].get(); }

		D3D12CommandQueue* GetQueue(rhi::CommandQueueType queue) { return this->m_queues[queue].get(); }
	public:
		operator ID3D12Device*() const { return this->m_d3dDevice.Get(); }
		operator ID3D12Device2* () const { return this->m_d3dDevice2.Get(); }
		operator ID3D12Device5* () const { return this->m_d3dDevice5.Get(); }

	private:
		void CreateDevice(Config const& config);
		void CreateDeviceResources(Config const& config);
		void CreateSwapChain(uint32_t width, uint32_t height);
		void InitializeResoucePools();
		void FinalizeResourcePools();

		void RunGarbageCollection(size_t frameCount);

	private:
		uint32_t m_frameCount = 0;
		DeviceCapabilities m_capabilities = {};
		std::unique_ptr<CommandContextManager> m_commandContextManager;
		std::shared_ptr<DescriptorAllocationHanlder> m_descriptorAllocator;

		// -- Direct3D objects ---
		Microsoft::WRL::ComPtr<IDXGIAdapter1> m_gpuAdapter;
		Microsoft::WRL::ComPtr<ID3D12Device> m_d3dDevice;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_d3dDevice2;
		Microsoft::WRL::ComPtr<ID3D12Device5> m_d3dDevice5;
		core::EnumArray<rhi::CommandQueueType, std::unique_ptr<D3D12CommandQueue>> m_queues;

		// -- MA Objects ---
		Microsoft::WRL::ComPtr<D3D12MA::Allocator> m_gpuMemAllocator;

		// -- Swap chain objects ---
		Microsoft::WRL::ComPtr<IDXGIFactory6> m_dxgiFactory;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
		std::vector<rhi::TextureHandle> m_backBuffers;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;

		// -- Resource Pools ---
		HandlePool<D3D12Texture, Texture> m_texturePool;

		// -- Cached device properties. ---
		D3D_FEATURE_LEVEL m_d3dFeatureLevel;
		D3D12_FEATURE_DATA_ROOT_SIGNATURE m_featureDataRootSignature = {};
		D3D12_FEATURE_DATA_SHADER_MODEL	m_featureDataShaderModel = {};
		ShaderModel m_minShaderModel = ShaderModel::SM_6_6;
		DWORD m_dxgiFactoryFlags;
		core::WindowHandle m_window = NULL;
		Rect m_outputSize;
		DXGI_FORMAT m_backBufferFormat;
		uint32_t m_backBufferCount = 3;

		bool m_isUnderGraphicsDebugger = false;
		D3D_FEATURE_LEVEL m_d3dMinFeatureLevel = {};
		
		bool m_allowTearing : 1;
		bool m_vSyncEnabled : 1;
	};

}