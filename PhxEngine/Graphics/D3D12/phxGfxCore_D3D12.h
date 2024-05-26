#pragma once

#include "Graphics/phxGfxDescriptors.h"

namespace phx::gfx
{
	struct IDeviceNotify;
}

namespace phx::gfx::d3d12
{
	// Helper class for COM exceptions
	class com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr) noexcept : result(hr) {}

		const char* what() const noexcept override
		{
			static char s_str[64] = {};
			sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
			return s_str;
		}

	private:
		HRESULT result;
	};

	// Helper utility converts D3D API failures into exceptions.
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch DirectX API errors
			throw com_exception(hr);
		}
	}

	struct D3D12Context
	{
		static void Initialize(ContextDesc const& desc);
		static bool ResizeSwapchain(int width, int height);
		static void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
		static void SubmitFrame();
		static void WaitForGpu();
		static void Finalize();

		static constexpr size_t MAX_BACK_BUFFER_COUNT = 3;

		inline static ContextDesc										  Desc = {};
		inline static DeviceCapabilities								  DeviceCapabilities = {};
		inline static uint32_t                                            BackBufferIndex = 0;

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
		inline static uint32_t                                              BackBufferCount = 0;
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
}

