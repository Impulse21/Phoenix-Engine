#pragma once

#include "RHI/RHIFactory.h"
#include "RHID3D12/GraphicsDevice.h"

namespace PhxEngine::RHI::D3D12
{
	class D3D12RHI : public IRHI
	{
		static D3D12RHI* SingleD3DRHI;

	public:
		D3D12RHI(DXGIGpuAdapter const& adapter);
		~D3D12RHI() = default;

		void Initialize() override;
		void Finalize() override;

	private:
		DXGIGpuAdapter m_adapter;
	};

	class D3D12RHIFactory : public IRHIFactory
	{
	public:
		bool IsSupported() override { return this->IsSupported(FeatureLevel::SM6); }
		bool IsSupported(FeatureLevel requestedFeatureLevel) override;
		std::unique_ptr<PhxEngine::RHI::IRHI> CreateRHI() override;

	private:
		void FindAdapter();
		RefCountPtr<IDXGIFactory6> CreateDXGIFactory6() const;


	private:
		// Choosen adapter.
		std::shared_ptr<DXGIGpuAdapter> m_choosenAdapter;
	};
}


