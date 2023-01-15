#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "D3D12Viewport.h"
#include "D3D12RHI.h"
#include "D3D12Adapter.h"
#include "D3D12CommandQueue.h"

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;


RHIViewportHandle D3D12RHI::CreateViewport(RHIViewportDesc const& desc)
{
	std::unique_ptr<D3D12Viewport> viewportImpl = std::make_unique<D3D12Viewport>(desc, this->GetAdapter());
	viewportImpl->Initialize();

	return RHIViewportHandle::Create(viewportImpl.release());
}

RHIShaderHandle D3D12RHI::CreateShader(RHIShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode)
{
	// auto shaderImpl = std::make_unique<Shader>(desc, shaderByteCode, shaderByteCode.Size());
	return nullptr;
}

RHIGraphicsPipelineHandle PhxEngine::RHI::D3D12::D3D12RHI::CreateGraphicsPipeline(RHIGraphicsPipelineDesc const& desc)
{
	return nullptr;
}

IRHIFrameRenderCtx* PhxEngine::RHI::D3D12::D3D12RHI::BeginFrameRenderContext(RHIViewportHandle viewport)
{
	return nullptr;
}

void PhxEngine::RHI::D3D12::D3D12RHI::FinishAndPresetFrameRenderContext(IRHIFrameRenderCtx* context)
{
}

void PhxEngine::RHI::D3D12::D3D12Viewport::Initialize()
{
	D3D12Adapter* adapter = this->GetParentAdapter();
	IDXGIFactory2* factory6 = adapter->GetDxgiFactory6();

	assert(this->m_desc.WindowHandle);
	if (!this->m_desc.WindowHandle)
	{
		LOG_CORE_ERROR("Unable to create viewport. Invalid window handle");
		throw std::runtime_error("Invalid Window Error");
	}

	auto& mapping = GetDxgiFormatMapping(this->m_desc.Format);

	DXGI_SWAP_CHAIN_DESC1 dx12Desc = {};
	dx12Desc.Width = this->m_desc.Width;
	dx12Desc.Height = this->m_desc.Height;
	dx12Desc.Format = mapping.RtvFormat;
	dx12Desc.SampleDesc.Count = 1;
	dx12Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dx12Desc.BufferCount = this->m_desc.BufferCount;
	dx12Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dx12Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ThrowIfFailed(
		factory6->CreateSwapChainForHwnd(
			adapter->GetGfxQueue()->GetD3D12CommandQueue(),
			static_cast<HWND>(this->m_desc.WindowHandle),
			&dx12Desc,
			nullptr,
			nullptr,
			this->m_swapchain.ReleaseAndGetAddressOf()));

	ThrowIfFailed(this->m_swapchain->QueryInterface(this->m_swapchain4.ReleaseAndGetAddressOf()));
}
