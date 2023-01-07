#pragma once

#include "RHI/RHIFactory.h"
#include "RHID3D12/GraphicsDevice.h"

namespace PhxEngine::RHI::D3D12
{
	class D3D12RHIFactory : public IRHIFactory
	{
	public:
		bool IsSupported() override { this->IsSupported(FeatureLevel::SM6); }
		bool IsSupported(FeatureLevel requestedFeatureLevel) override;
		std::unique_ptr<PhxEngine::RHI::IGraphicsDevice> CreateRHI() override;

	private:
		void FindAdapter();
		Microsoft::WRL::ComPtr<IDXGIFactory6> CreateDXGIFactory6() const;


	private:
		// Choosen adapter.
		std::shared_ptr<DXGIGpuAdapter> m_choosenAdapter;
	};
}

class D3D12RHI
{
};

