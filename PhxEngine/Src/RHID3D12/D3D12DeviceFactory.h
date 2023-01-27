#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include "D3D12Common.h"
#include "RHI/GfxDeviceFactory.h"

namespace PhxEngine::RHI::D3D12
{
	class D3D12Adapter;
	class D3D12GraphicsDevice;

	class D3D12GraphicsDeviceFactory : public IGraphicsDeviceFactory
	{
	public:
		std::unique_ptr<IGraphicsDevice> CreateDevice() override;

	private:
		void FindAdapter(Microsoft::WRL::ComPtr<IDXGIFactory6> factory, D3D12Adapter& outAdapter);
		Microsoft::WRL::ComPtr<IDXGIFactory6> CreateDXGIFactory6() const;

		bool IsSupported() override { return this->IsSupported(FeatureLevel::SM6); }
		bool IsSupported(FeatureLevel requestedFeatureLevel) override;
	};


}