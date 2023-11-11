#define NOMINMAX
#include <RHI/PhxRHI.h>

#include "D3D12Common.h"

#include <Core/Log.h>


#include <Core/Memory.h>
#include "D3D12Context.h"
#include "DxgiFormatMapping.h"

#include "D3D12Resources.h"
#include "Core/String.h"
#include <stdexcept>

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

// -- D3D12 Implementation

namespace
{
	// -- Globals
	uint64_t FrameCount = 0;
	uint64_t BufferCount = 0;

	std::array<Core::RefCountPtr<ID3D12Fence>, (int)RHI::CommandListType::Count> FrameFences;

	bool SafeTestD3D12CreateDevice(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL minFeatureLevel, D3D12DeviceBasicInfo& outInfo)
	{
#pragma warning(disable:6322)
		try
		{
			ID3D12Device* Device = nullptr;
			const HRESULT d3D12CreateDeviceResult = D3D12CreateDevice(adapter, minFeatureLevel, IID_PPV_ARGS(&Device));
			if (SUCCEEDED(d3D12CreateDeviceResult))
			{
				outInfo.NumDeviceNodes = Device->GetNodeCount();

				Device->Release();
				return true;
			}
			else
			{
				LOG_RHI_WARN("D3D12CreateDevice failed.");
			}
		}
		catch (...)
		{
		}
#pragma warning(default:6322)

		return {};
	}

	inline D3D12_RESOURCE_STATES ConvertResourceStates(RHI::ResourceStates stateBits)
	{
		if (stateBits == ResourceStates::Common)
			return D3D12_RESOURCE_STATE_COMMON;

		D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON; // also 0

		if ((stateBits & ResourceStates::ConstantBuffer) != 0) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if ((stateBits & ResourceStates::VertexBuffer) != 0) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if ((stateBits & ResourceStates::IndexGpuBuffer) != 0) result |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if ((stateBits & ResourceStates::IndirectArgument) != 0) result |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		if ((stateBits & ResourceStates::ShaderResource) != 0) result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if ((stateBits & ResourceStates::UnorderedAccess) != 0) result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if ((stateBits & ResourceStates::RenderTarget) != 0) result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		if ((stateBits & ResourceStates::DepthWrite) != 0) result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		if ((stateBits & ResourceStates::DepthRead) != 0) result |= D3D12_RESOURCE_STATE_DEPTH_READ;
		if ((stateBits & ResourceStates::StreamOut) != 0) result |= D3D12_RESOURCE_STATE_STREAM_OUT;
		if ((stateBits & ResourceStates::CopyDest) != 0) result |= D3D12_RESOURCE_STATE_COPY_DEST;
		if ((stateBits & ResourceStates::CopySource) != 0) result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		if ((stateBits & ResourceStates::ResolveDest) != 0) result |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
		if ((stateBits & ResourceStates::ResolveSource) != 0) result |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		if ((stateBits & ResourceStates::Present) != 0) result |= D3D12_RESOURCE_STATE_PRESENT;
		if ((stateBits & ResourceStates::AccelStructRead) != 0) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if ((stateBits & ResourceStates::AccelStructWrite) != 0) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if ((stateBits & ResourceStates::AccelStructBuildInput) != 0) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if ((stateBits & ResourceStates::AccelStructBuildBlas) != 0) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if ((stateBits & ResourceStates::ShadingRateSurface) != 0) result |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
		if ((stateBits & ResourceStates::GenericRead) != 0) result |= D3D12_RESOURCE_STATE_GENERIC_READ;
		if ((stateBits & ResourceStates::ShaderResourceNonPixel) != 0) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

		return result;
	}

	constexpr D3D12_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
	{
		D3D12_SUBRESOURCE_DATA data = {};
		data.pData = pInitialData.pData;
		data.RowPitch = pInitialData.rowPitch;
		data.SlicePitch = pInitialData.slicePitch;

		return data;
	}
}

bool PhxEngine::RHI::Initialize(RHIParams const& params)
{
	FrameCount = 0;
	BufferCount = params.NumInflightFrames;
    D3D12Adapter selectedAdapter = {};
    {
        Core::RefCountPtr<IDXGIFactory6> factory;
        ThrowIfFailed(
            CreateDXGIFactory2(0, IID_PPV_ARGS(factory.ReleaseAndGetAddressOf())));

		Core::RefCountPtr<IDXGIAdapter1> tempAdapter;
		for (uint32_t adapterIndex = 0; EnumAdapters(adapterIndex, factory.Get(), tempAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
		{
			if (!tempAdapter)
			{
				continue;
			}

			DXGI_ADAPTER_DESC1 desc = {};
			tempAdapter->GetDesc1(&desc);

			std::string name;
			Core::StringConvert(desc.Description, name);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				LOG_RHI_INFO("GPU '%s' is a software adapter. Skipping consideration as this is not supported.", name.c_str());
				continue;
			}

			D3D12DeviceBasicInfo basicDeviceInfo = {};
			if (!SafeTestD3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_11_1, basicDeviceInfo))
			{
				continue;
			}

			if (basicDeviceInfo.NumDeviceNodes > 1)
			{
				LOG_RHI_INFO("GPU '%s' has one or more device nodes. Currently only support one device ndoe.", name.c_str());
			}

			if (!selectedAdapter.NativeAdapter || selectedAdapter.DedicatedVideoMemory < desc.DedicatedVideoMemory)
			{
				selectedAdapter.Name = name;
				selectedAdapter.NativeAdapter = tempAdapter;
				selectedAdapter.DedicatedVideoMemory = desc.DedicatedVideoMemory;
				selectedAdapter.DedicatedVideoMemory = desc.DedicatedVideoMemory;
				selectedAdapter.DedicatedSystemMemory = desc.DedicatedSystemMemory;
				selectedAdapter.SharedSystemMemory = desc.SharedSystemMemory;
				selectedAdapter.BasicDeviceInfo = basicDeviceInfo;
			}
		}

		if (!selectedAdapter.NativeAdapter)
		{
			LOG_RHI_ERROR("No suitable adapters were found - Unable to initialize RHI.");
			return {};
		}
    }

	LOG_RHI_INFO(
		"Found Suitable D3D12 Adapter '%s'",
		selectedAdapter.Name.c_str());

	LOG_RHI_INFO(
		"Adapter has %dMB of dedicated video memory, %dMB of dedicated system memory, and %dMB of shared system memory.",
		selectedAdapter.DedicatedVideoMemory / (1024 * 1024),
		selectedAdapter.DedicatedSystemMemory / (1024 * 1024),
		selectedAdapter.SharedSystemMemory / (1024 * 1024));

	
	// Could be a static interface.
	Context::Initialize(selectedAdapter, BufferCount);

	// Create Frame Fences
	for (size_t q = 0; q < FrameFences.size(); q++)
	{
		// Create a frame fence per queue;
		ThrowIfFailed(
			Context::D3d12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&FrameFences[q])));
	}

    return true;
}

bool PhxEngine::RHI::Finalize()
{
	Context::Finalize();
    return {};
}

void PhxEngine::RHI::Present(SwapChain const& swapchainToPresent)
{
	// -- Mark Queues for completion ---
	for (size_t q = 0; q < (size_t)CommandListType::Count; ++q)
	{
		D3D12CommandQueue& queue = Context::Queue(static_cast<CommandListType>(q));

		// Single Frame Fence to the frame it's currently doing work for.
		queue.GetD3D12CommandQueue()->Signal(FrameFences[q].Get(), FrameCount);
	}

	// -- Present SwapChain ---
	{
		UINT presentFlags = 0;
		const bool enableVSync = swapchainToPresent.Desc().VSync;

		if (!enableVSync && !swapchainToPresent.Desc().Fullscreen)
		{
			presentFlags = DXGI_PRESENT_ALLOW_TEARING;
		}
		const D3D12SwapChain& platformSwapChain = swapchainToPresent.PlatformResource();
		HRESULT hr = platformSwapChain.NativeSwapchain4->Present((UINT)enableVSync, presentFlags);

		// If the device was reset we must completely reinitialize the renderer.
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{

#ifdef _DEBUG
			char buff[64] = {};
			sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
				static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED)
					? Context::D3d12Device()->GetDeviceRemovedReason()
					: hr));

			LOG_RHI_ERROR(buff);
#endif

			// TODO: Handle device lost
			// HandleDeviceLost();
		}
	}
	// Begin the next frame - this affects the GetCurrentBackBufferIndex()
	FrameCount++;

	// -- Wait for next frame ---
	{
		for (size_t q = 0; q < (size_t)CommandListType::Count; ++q)
		{
			const UINT64 completedFrame = FrameFences[q]->GetCompletedValue();

			// If the fence is max uint64, that might indicate that the device has been removed as fences get reset in this case
			assert(completedFrame != UINT64_MAX);
			const uint64_t bufferCount = BufferCount;

			// Since our frame count is 1 based rather then 0, increment the number of buffers by 1 so we don't have to wait on the first 3 frames
			// that are kicked off.
			if (FrameCount >= (bufferCount + 1) && completedFrame < FrameCount)
			{
				// Wait on the frames last value?
				// NULL event handle will simply wait immediately:
				//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion#remarks
				ThrowIfFailed(
					FrameFences[q]->SetEventOnCompletion(FrameCount - bufferCount, nullptr));
			}
		}

		
	}
}

void PhxEngine::RHI::WaitForIdle()
{
	Context::WaitForIdle();
}

CommandList* PhxEngine::RHI::BeginCommandList(RHI::CommandListType type)
{
	return nullptr;
}

uint64_t PhxEngine::RHI::SubmitCommands(CommandList* commandList)
{
	return 0;
}

bool PhxEngine::RHI::Factory::CreateSwapChain(SwapchainDesc const& desc, void* windowHandle, SwapChain& out)
{
	PlatformSwapChain& impl = out.PlatformResource();

	const auto& formatMapping = GetDxgiFormatMapping(desc.Format);
	if (impl.NativeSwapchain)
	{
		D3D12::Context::WaitForIdle();

		UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		ThrowIfFailed(
			impl.NativeSwapchain->ResizeBuffers(
				D3D12::Context::MaxFramesInflight(),
				desc.Width,
				desc.Height,
				formatMapping.RtvFormat,
				swapChainFlags));
	}
	else
	{
		HRESULT hr;
		UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		// Create swapchain:
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = desc.Width;
		swapChainDesc.Height = desc.Height;
		swapChainDesc.Format = formatMapping.RtvFormat;
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = Context::MaxFramesInflight();
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags = swapChainFlags;

		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
		fullscreenDesc.Windowed = !desc.Fullscreen;

		hr = Context::DxgiFctory6()->CreateSwapChainForHwnd(
			Context::Queue(CommandListType::Graphics).GetD3D12CommandQueue(),
			static_cast<HWND>(windowHandle),
			&swapChainDesc,
			&fullscreenDesc,
			nullptr,
			impl.NativeSwapchain.GetAddressOf()
		);

		if (FAILED(hr))
		{
			return false;
		}

		if (FAILED(impl.NativeSwapchain->QueryInterface(IID_PPV_ARGS(&impl.NativeSwapchain4))))
		{
			return false;
		}
	}

	impl.Release();

	impl.BackBuffers.resize(Context::MaxFramesInflight());
	impl.BackBuferViews.resize(Context::MaxFramesInflight());

	assert(false); // Deferred Release
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = D3D12::GetDxgiFormatMapping(desc.Format).RtvFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	for (int i = 0; i < impl.BackBuffers.size(); i++)
	{
		ThrowIfFailed(
			impl.NativeSwapchain4->GetBuffer(i, IID_PPV_ARGS(&impl.BackBuffers[i])));

		D3D12Descriptor& descriptor = impl.BackBuferViews[i];
		descriptor.Allocation = D3D12::Context::CpuRenderTargetHeap().Allocate(1);

		Context::D3d12Device()->CreateRenderTargetView(impl.BackBuffers[i].Get(), &rtvDesc, descriptor.GetView());
	}

	return true;
}

bool PhxEngine::RHI::Factory::CreateTexture(TextureDesc const& desc, Texture& out)
{
	out.m_desc = desc;
	PlatformTexture& textureImpl = out.PlatformResource();

	D3D12_CLEAR_VALUE d3d12OptimizedClearValue = {};
	d3d12OptimizedClearValue.Color[0] = desc.OptmizedClearValue.Colour.R;
	d3d12OptimizedClearValue.Color[1] = desc.OptmizedClearValue.Colour.G;
	d3d12OptimizedClearValue.Color[2] = desc.OptmizedClearValue.Colour.B;
	d3d12OptimizedClearValue.Color[3] = desc.OptmizedClearValue.Colour.A;
	d3d12OptimizedClearValue.DepthStencil.Depth = desc.OptmizedClearValue.DepthStencil.Depth;
	d3d12OptimizedClearValue.DepthStencil.Stencil = desc.OptmizedClearValue.DepthStencil.Stencil;

	auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
	d3d12OptimizedClearValue.Format = dxgiFormatMapping.RtvFormat;

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;

	if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		if ((desc.BindingFlags & BindingFlags::ShaderResource) != BindingFlags::ShaderResource)
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
	}
	if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	CD3DX12_RESOURCE_DESC resourceDesc = {};

	const bool isTypeless = (desc.MiscFlags | RHI::TextureMiscFlags::Typeless) == RHI::TextureMiscFlags::Typeless;
	switch (desc.Dimension)
	{
	case TextureDimension::Texture1D:
	case TextureDimension::Texture1DArray:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex1D(
				isTypeless ? dxgiFormatMapping.ResourceFormat : dxgiFormatMapping.RtvFormat,
				desc.Width,
				desc.ArraySize,
				desc.MipLevels,
				resourceFlags);
		break;
	}
	case TextureDimension::Texture2D:
	case TextureDimension::Texture2DArray:
	case TextureDimension::TextureCube:
	case TextureDimension::TextureCubeArray:
	case TextureDimension::Texture2DMS:
	case TextureDimension::Texture2DMSArray:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex2D(
				isTypeless ? dxgiFormatMapping.ResourceFormat : dxgiFormatMapping.RtvFormat,
				desc.Width,
				desc.Height,
				desc.ArraySize,
				desc.MipLevels,
				1,
				0,
				resourceFlags);
		break;
	}
	case TextureDimension::Texture3D:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex3D(
				isTypeless ? dxgiFormatMapping.ResourceFormat : dxgiFormatMapping.RtvFormat,
				desc.Width,
				desc.Height,
				desc.ArraySize,
				desc.MipLevels,
				resourceFlags);
		break;
	}
	default:
		throw std::runtime_error("Unsupported texture dimension");
	}

	const bool useClearValue =
		((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget) ||
		((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil);

	textureImpl.TotalSize = 0;
	textureImpl.Footprints.resize(desc.ArraySize * std::max(uint16_t(1u), desc.MipLevels));
	textureImpl.RowSizesInBytes.resize(textureImpl.Footprints.size());
	textureImpl.NumRows.resize(textureImpl.Footprints.size());
	Context::D3d12Device2()->GetCopyableFootprints(
		&resourceDesc,
		0,
		(UINT)textureImpl.Footprints.size(),
		0,
		textureImpl.Footprints.data(),
		textureImpl.NumRows.data(),
		textureImpl.RowSizesInBytes.data(),
		&textureImpl.TotalSize
	);

	ThrowIfFailed(
		Context::D3d12MemAllocator()->CreateResource(
			&allocationDesc,
			&resourceDesc,
			ConvertResourceStates(desc.InitialState),
			useClearValue ? &d3d12OptimizedClearValue : nullptr,
			&textureImpl.Allocation,
			IID_PPV_ARGS(&textureImpl.D3D12Resource)));


	std::wstring debugName;
	Core::StringConvert(desc.DebugName, debugName);
	textureImpl.D3D12Resource->SetName(debugName.c_str());

	// Create Data
	return true;
}

bool PhxEngine::RHI::Factory::CreateGpuBuffer(BufferDesc const& desc, Buffer& out)
{
	return false;
}

bool PhxEngine::RHI::Factory::CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride)
{
	return false;
}

bool PhxEngine::RHI::Factory::CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode)
{
	return false;
}

bool PhxEngine::RHI::Factory::CreateInputLayout(Core::Span<VertexAttributeDesc> descs)
{
	return false;
}

bool PhxEngine::RHI::Factory::CreateGfxPipeline(GfxPipelineDesc const& desc)
{
	return false;
}

bool PhxEngine::RHI::Factory::CreateComputePipeline(ComputePipelineDesc const& desc)
{
	return false;
}

bool PhxEngine::RHI::Factory::CreateMeshPipeline(MeshPipelineDesc const& desc)
{
	return false;
}
