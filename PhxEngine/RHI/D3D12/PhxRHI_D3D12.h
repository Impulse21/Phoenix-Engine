#pragma once

#include "RHI/phxRHI.h"
#include "Core/phxHandlePool.h"

namespace phx::rhi
{
	class D3D12GfxDevice;

	class D3D12GfxDeviceChild
	{
	public:
		D3D12GfxDeviceChild(D3D12GfxDevice* gfxDevice)
			: m_gfxDevice(gfxDevice)
		{}

	protected:
		D3D12GfxDevice* m_gfxDevice;
	};
	class D3D12ResourceManager final : D3D12GfxDeviceChild
	{
	public:
		D3D12ResourceManager(D3D12GfxDevice* gfxDevice)
			: D3D12GfxDeviceChild(gfxDevice)
		{}

	private:
	};

	class D3D12GfxDevice : public GfxDevice
	{
	public:
		static inline GfxDevice* Ptr = nullptr;
		D3D12GfxDevice(Config const& config);
		~D3D12GfxDevice();

	private:
		void InitializeDeviceResources(Config const& config);

	private:
		D3D12ResourceManager m_resourceManager;

		Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;

	};

}