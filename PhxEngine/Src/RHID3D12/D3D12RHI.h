#pragma once

#include "RHI/RHIFactory.h"

#include "RHID3D12/GraphicsDevice.h"

namespace PhxEngine::RHI::D3D12
{
	// Forward declares
	class D3D12Viewport;
	class D3D12Adapter;
	class D3D12CommandList;
	class D3D12Device;

	class D3D12Shader : public RefCounter<IRHIShader>
	{
	public:
		D3D12Shader(RHIShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode)
			: m_desc(desc) 
		{
			this->m_byteCode.resize(shaderByteCode.Size());
			std::memcpy(this->m_byteCode.data(), shaderByteCode.begin(), shaderByteCode.Size());
		};

		~D3D12Shader() = default;

		const RHIShaderDesc& GetDesc() const override { return this->m_desc;  }
		const std::vector<uint8_t>& GetByteCode() const { return this->m_byteCode; }
		RefCountPtr<ID3D12RootSignatureDeserializer> GetRootSigDeserializer() { return this->m_rootSignatureDeserializer; }

		void SetRootSignature(RefCountPtr<ID3D12RootSignature> rootSig) { this->m_rootSignature = rootSig; };
		RefCountPtr<ID3D12RootSignature> GetRootSignature() const { return this->m_rootSignature; };

	private:
		const RHIShaderDesc m_desc;
		std::vector<uint8_t> m_byteCode;
		RefCountPtr<ID3D12RootSignatureDeserializer> m_rootSignatureDeserializer;
		RefCountPtr<ID3D12RootSignature> m_rootSignature;
	};

	class D3D12GraphicsPipeline : public RefCounter<IRHIGraphicsPipeline>
	{
	public:
		D3D12GraphicsPipeline(RHIGraphicsPipelineDesc const& desc)
			: m_desc(desc) { };
		~D3D12GraphicsPipeline() = default;

		RefCountPtr<ID3D12RootSignature> D3D12RootSignature;
		RefCountPtr<ID3D12PipelineState> D3D12PipelineState;

		const RHIGraphicsPipelineDesc& GetDesc() const override { return this->m_desc; }

	private:
		const RHIGraphicsPipelineDesc m_desc;
	};

	class D3D12InputLayout : public RefCounter<IRHIInputLayout>
	{
	public:
		D3D12InputLayout(Core::Span<RHIVertexAttributeDesc> vertexAttributes)
		{
			this->Attributes.resize(vertexAttributes.Size());

			std::memcpy(this->Attributes.data(), vertexAttributes.begin(), vertexAttributes.Size());
		};

		~D3D12InputLayout() = default;

		[[nodiscard]] virtual uint32_t GetNumAttributes() const override { return this->Attributes.size(); };
		[[nodiscard]] virtual const RHIVertexAttributeDesc* GetAttributeDesc(uint32_t index) const override
		{
			assert(index < this->GetNumAttributes());
			return &this->Attributes[index];
		}

	public:
		std::vector<RHIVertexAttributeDesc> Attributes;
		std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements; 
		
		// maps a binding slot to an element stride
		std::unordered_map<uint32_t, uint32_t> ElementStrides;
	};


	class D3D12FrameRenderCtx : public IRHIFrameRenderCtx
	{
	public:
		D3D12FrameRenderCtx() = default;
		~D3D12FrameRenderCtx() = default;

		IRHICommandList* BeginCommandRecording(RHICommandQueueType QueueType = RHICommandQueueType::Graphics) override;
		uint64_t SubmitCommands(Core::Span<IRHICommandList*> commands) override;
		RHITextureHandle GetBackBuffer() override { return this->m_backBuffer; }

		D3D12Viewport* GetViewport() { return this->m_viewport; }

	private:
		D3D12Viewport* m_viewport = nullptr;
		D3D12Device* m_device = nullptr;
		RHITextureHandle m_backBuffer;

		std::vector<D3D12CommandList> m_commandLists;
		std::queue<D3D12CommandList*> m_availableCommandLists;
		std::mutex m_commandMutex;
	};

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
		RHIInputLayoutHandle CreateInputLayout(Core::Span<RHIVertexAttributeDesc> desc) override;
		RHIGraphicsPipelineHandle CreateGraphicsPipeline(RHIGraphicsPipelineDesc const& desc) override;

		IRHIFrameRenderCtx& BeginFrameRenderContext(RHIViewportHandle viewport) override;
		void FinishAndPresetFrameRenderContext(IRHIFrameRenderCtx* context) override;

	public:
		D3D12Adapter* GetAdapter() { return this->m_adapter.get(); }

	private:
		void TranslateBlendState(RHIBlendRenderState const& inState, D3D12_BLEND_DESC& outState);
		void TranslateDepthStencilState(RHIDepthStencilRenderState const& inState, D3D12_DEPTH_STENCIL_DESC& outState);
		void TranslateRasterState(RHIRasterRenderState const& inState, D3D12_RASTERIZER_DESC& outState);

	private:
		std::shared_ptr<D3D12Adapter> m_adapter;
		D3D12FrameRenderCtx m_frameContext;
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


