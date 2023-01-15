#pragma once

#include "RHI/RHIFactory.h"
#include "RHID3D12/GraphicsDevice.h"

namespace PhxEngine::RHI::D3D12
{
	class D3D12Adapter;
	class D3D12RHI : public IPhxRHI
	{
		static D3D12RHI* Singleton;

	public:
		D3D12RHI(std::shared_ptr<D3D12Adapter>& adapter);
		~D3D12RHI() = default;

		void Initialize() override;
		void Finalize() override;

		RHIViewportHandle CreateViewport(RHIViewportDesc const& desc) override;
		RHIShaderHandle CreateShader(RHIShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode) override;
		RHIGraphicsPipelineHandle CreateGraphicsPipeline(RHIGraphicsPipelineDesc const& desc) override;

		IRHIFrameRenderCtx* BeginFrameRenderContext(RHIViewportHandle viewport) override;
		void FinishAndPresetFrameRenderContext(IRHIFrameRenderCtx* context) override;

	public:
		D3D12Adapter* GetAdapter() { return this->m_adapter.get(); }

	private:
		std::shared_ptr<D3D12Adapter> m_adapter;

		bool m_isUnderGraphicsDebugger = false;
	};

	class D3D12RHIFactory : public IRHIFactory
	{
	public:
		bool IsSupported() override { return this->IsSupported(FeatureLevel::SM6); }
		bool IsSupported(FeatureLevel requestedFeatureLevel) override;
		std::unique_ptr<PhxEngine::RHI::IPhxRHI> CreateRHI() override;

	private:
		void FindAdapter();
		RefCountPtr<IDXGIFactory6> CreateDXGIFactory6() const;


	private:
		// Choosen adapter.
		std::shared_ptr<D3D12Adapter> m_choosenAdapter;
	};
}


