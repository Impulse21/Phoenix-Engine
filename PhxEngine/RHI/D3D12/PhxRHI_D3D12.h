#pragma once
#include "Core/CommonInclude.h"
#include "RHI/PhxRHIDescriptors.h"

namespace phx::rhi
{
	class IDeviceNotify;
	struct RHIDesc;
}

namespace phx::rhi::d3d12
{

	struct D3D12Context
	{
		static void Initialize(RHIDesc const& desc);
		static bool ResizeSwapchain(int width, int height);
		static void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		static void WaitForGpu();
		static void Finalize();

		static constexpr size_t MAX_BACK_BUFFER_COUNT = 3;

		inline static RHIDesc											  Desc = {};
		inline static DeviceCapabilities								  DeviceCapabilities = {};
		inline static uint32                                              BackBufferIndex = 0;

		// Direct3D objects.
		inline static Microsoft::WRL::ComPtr<ID3D12Device>                D3dDevice;
		inline static Microsoft::WRL::ComPtr<ID3D12Device>                D3dDevice2;
		inline static Microsoft::WRL::ComPtr<ID3D12Device>                D3dDevice5;
		inline static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   CommandList;
		inline static Microsoft::WRL::ComPtr<ID3D12CommandQueue>          CommandQueue;
		inline static Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      CommandAllocators[MAX_BACK_BUFFER_COUNT];

		// Swap chain objects.
		inline static Microsoft::WRL::ComPtr<IDXGIFactory6>               DxgiFactory;
		inline static Microsoft::WRL::ComPtr<IDXGISwapChain3>             SwapChain;
		inline static Microsoft::WRL::ComPtr<ID3D12Resource>              RenderTargets[MAX_BACK_BUFFER_COUNT];
		inline static Microsoft::WRL::ComPtr<ID3D12Resource>              DepthStencil;

		// Presentation fence objects.
		inline static Microsoft::WRL::ComPtr<ID3D12Fence>                 Fence;
		inline static UINT64                                              FenceValues[MAX_BACK_BUFFER_COUNT];
		inline static Microsoft::WRL::Wrappers::Event                     FenceEvent;

		// Direct3D rendering objects.
		inline static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        RtvDescriptorHeap;
		inline static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        DsvDescriptorHeap;
		inline static UINT                                                RtvDescriptorSize = 0;
		inline static D3D12_VIEWPORT                                      ScreenViewport = {};
		inline static D3D12_RECT                                          ScissorRect = {};

		// Direct3D properties.
		inline static DXGI_FORMAT                                         BackBufferFormat;
		inline static DXGI_FORMAT                                         DepthBufferFormat;
		inline static uint32                                              BackBufferCount = 0;
		inline static D3D_FEATURE_LEVEL                                   D3dMinFeatureLevel;

		// Cached device properties.
		inline static HWND                                                Window = NULL;
		inline static D3D_FEATURE_LEVEL                                   D3dFeatureLevel;
		inline static D3D12_FEATURE_DATA_ROOT_SIGNATURE					  FeatureDataRootSignature = {};
		inline static D3D12_FEATURE_DATA_SHADER_MODEL					  FeatureDataShaderModel = {};
		inline static ShaderModel										  MinShaderModel = ShaderModel::SM_6_6;
		inline static DWORD                                               DxgiFactoryFlags;
		inline static RECT                                                OutputSize;

		// HDR Support
		inline static DXGI_COLOR_SPACE_TYPE                               ColorSpace;

		// DeviceResources options (see flags above)
		inline static unsigned int                                        Options;

		// The IDeviceNotify can be held directly as it owns the DeviceResources.
		inline static IDeviceNotify* DeviceNotify = nullptr;
		inline static bool IsUnderGraphicsDebugger = false;
	};

	struct D3D12Buffer
	{
#if false
		DeferredReleasePtr<ID3D12Resource> Resource;
		D3D12_GPU_VIRTUAL_ADDRESS Address;
		UINT64 Offset = 0;
		LockType CurrentLock = LockType::None;

		explicit operator bool() const
		{
			return !!Resource;
		}

		void Release()
		{
			Resource.Reset();
		}

		bool CreateConstantBufferView(
			const ResourceDesc& desc,
			const BindingSchema binding_schema,
			const BaseRootSignatureParameter bind_point,
			D3D12ConstantBufferView* view) const
		{
			UNREFERENCED_PARAMETER(desc);
			UNREFERENCED_PARAMETER(binding_schema);
			UNREFERENCED_PARAMETER(bind_point);
			view->Address = Address;
			return true;
		}

		bool CreateShaderView(const ResourceDesc& desc, D3D12_CPU_DESCRIPTOR_HANDLE* view) const;
		bool CreateUnorderedView(const ResourceDesc& desc, D3D12_CPU_DESCRIPTOR_HANDLE* view) const;

		uint8_t* Lock(const ResourceDesc& desc, const LockType type);
		void Unlock(const ResourceDesc& desc);

#else
		// Blank implementation
		explicit operator bool() const
		{
			return false;
		}

		void Release()
		{
			// no-op
		}
#endif
	};
}
