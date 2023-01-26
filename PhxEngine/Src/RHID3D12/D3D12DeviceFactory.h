#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/RHI/RefCountPtr.h>
#include "D3D12Common.h"
#include "RHI/GfxDeviceFactory.h"

namespace PhxEngine::RHI::D3D12
{
	class D3D12Adapter;
	class D3D12GraphicsDevice;

	class D3D12GraphicsDeviceFactory : public IGraphicsDeviceFactory
	{
	public:
		bool IsSupported() override { return this->IsSupported(FeatureLevel::SM6); }
		bool IsSupported(FeatureLevel requestedFeatureLevel) override;
		std::unique_ptr<IGraphicsDevice> CreateDevice() override;

	private:
		void FindAdapter();
		RefCountPtr<IDXGIFactory6> CreateDXGIFactory6() const;


	private:
		// Choosen adapter.
		std::shared_ptr<D3D12Adapter> m_choosenAdapter;
	};


}