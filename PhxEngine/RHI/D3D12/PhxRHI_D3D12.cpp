#include <PhxEngine.h>

#include <Core/Memory.h>
#include "D3D12Device.h"
#include "D3D12GpuMemoryAllocator.h"
#include "D3D12Resources.h"
#include "D3D12ResourceManager.h"

#include "Core/String.h"
#include <stdexcept>


using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

// -- D3D12 Implementation

namespace
{
	// -- Globals
	std::shared_ptr<D3D12Device> Device;
	std::shared_ptr<D3D12GpuMemoryAllocator> GpuAllocator;
	std::shared_ptr<D3D12ResourceManager> ResourceManager;

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
}

bool PhxEngine::RHI::Initialize(RHIParams const& params)
{
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

	
	Device = std::make_shared<D3D12Device>(selectedAdapter);
	GpuAllocator = std::make_shared<D3D12GpuMemoryAllocator>(Device);
	ResourceManager = std::make_shared<D3D12ResourceManager>(Device, GpuAllocator);

    return true;
}

bool PhxEngine::RHI::Finalize()
{
	ResourceManager->RunGrabageCollection();
	ResourceManager.reset();
	Device.reset();

    return {};
}

void PhxEngine::RHI::EndFrame(Core::Span<SwapChain> swapchainsToPresent)
{
}


GfxContext& PhxEngine::RHI::BeginGfxCtx()
{
    return {};
}

ComputeContext& PhxEngine::RHI::BeginComputeCtx()
{
    return {};
}

CopyContext& PhxEngine::RHI::BeginCopyCtx()
{
    return {};
}

SubmitRecipt PhxEngine::RHI::Submit(Core::Span<CommandContext> context)
{
    return SubmitRecipt();
}

CommandSignatureHandle PhxEngine::RHI::CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride)
{
    return {};
}

SwapChainHandle PhxEngine::RHI::CreateSwapChain(SwapchainDesc desc, void* windowHandle)
{
	out.m_desc = desc;
	HRESULT hr;

	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	const auto& formatMapping = GetDxgiFormatMapping(desc.Format);
	
	if (out.m_platformImpl == nullptr)
	{
		out.m_platformImpl = Core::RefCountPtr<PlatformSwapChain>::Create(phx_new(PlatformSwapChain));

		// Create swapchain:
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = desc.Width;
		swapChainDesc.Height = desc.Height;
		swapChainDesc.Format = formatMapping.RtvFormat;
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = desc.BufferCount;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags = swapChainFlags;

		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
		fullscreenDesc.Windowed = !desc.Fullscreen;

		hr = D3D12Context::DxgiFctory6->CreateSwapChainForHwnd(
			this->GetGfxQueue().GetD3D12CommandQueue(),
			static_cast<HWND>(windowHandle),
			&swapChainDesc,
			&fullscreenDesc,
			nullptr,
			this->m_swapChain.NativeSwapchain.GetAddressOf()
		);

		if (FAILED(hr))
		{
			throw std::exception();
		}

		hr = this->m_swapChain.NativeSwapchain.As(&this->m_swapChain.NativeSwapchain4);
		if (FAILED(hr))
		{
			throw std::exception();
		}
	}
	else
	{
		// Resize swapchain:
		this->WaitForIdle();

		// Delete back buffers
		for (int i = 0; i < this->m_swapChain.BackBuffers.size(); i++)
		{
			this->DeleteTexture(this->m_swapChain.BackBuffers[i]);
		}

		this->RunGarbageCollection(this->m_frameCount + 1);
		this->m_swapChain.BackBuffers.clear();

		hr = this->m_swapChain.NativeSwapchain4->ResizeBuffers(
			desc.BufferCount,
			desc.Width,
			desc.Height,
			formatMapping.RtvFormat,
			swapChainFlags
		);

		assert(SUCCEEDED(hr));
	}

	// -- From Wicked Engine
#ifdef ENABLE_HDR
	const bool hdr = desc->allow_hdr && IsSwapChainSupportsHDR(swapchain);

	// Ensure correct color space:
	//	https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HDR/src/D3D12HDR.cpp
	{
		internal_state->colorSpace = ColorSpace::SRGB; // reset to SDR, in case anything below fails to set HDR state
		DXGI_COLOR_SPACE_TYPE colorSpace = {};

		switch (desc->format)
		{
		case Format::R10G10B10A2_UNORM:
			// This format is either HDR10 (ST.2084), or SDR (SRGB)
			colorSpace = hdr ? DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
			break;
		case Format::R16G16B16A16_FLOAT:
			// This format is HDR (Linear):
			colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
			break;
		default:
			// Anything else will be SDR (SRGB):
			colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
			break;
		}

		UINT colorSpaceSupport = 0;
		if (SUCCEEDED(internal_state->swapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport)))
		{
			if (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
			{
				hr = internal_state->swapChain->SetColorSpace1(colorSpace);
				assert(SUCCEEDED(hr));
				if (SUCCEEDED(hr))
				{
					switch (colorSpace)
					{
					default:
					case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
						internal_state->colorSpace = ColorSpace::SRGB;
						break;
					case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
						internal_state->colorSpace = ColorSpace::HDR_LINEAR;
						break;
					case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
						internal_state->colorSpace = ColorSpace::HDR10_ST2084;
						break;
					}
				}
			}
		}
	}
#endif

	this->m_swapChain.BackBuffers.resize(desc.BufferCount);

	for (UINT i = 0; i < this->m_swapChain.Desc.BufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(
			this->m_swapChain.NativeSwapchain4->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		char allocatorName[32];
		sprintf_s(allocatorName, "Back Buffer %iu", i);

		this->m_swapChain.BackBuffers[i] = this->CreateTexture(
			{
				.BindingFlags = BindingFlags::RenderTarget,
				.Dimension = TextureDimension::Texture2D,
				.Format = this->m_swapChain.Desc.Format,
				.Width = this->m_swapChain.Desc.Width,
				.Height = this->m_swapChain.Desc.Height,
				.DebugName = std::string(allocatorName)
			},
			backBuffer);
	}
	return {};
}

ShaderHandle PhxEngine::RHI::CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode)
{
    return {};
}

InputLayoutHandle PhxEngine::RHI::CreateInputLayout(VertexAttributeDesc const& desc, uint32_t attributeCount)
{
    return {};
}

GfxPipelineHandle PhxEngine::RHI::CreateGfxPipeline(GfxPipelineDesc const& desc)
{
    return {};
}

ComputePipelineHandle PhxEngine::RHI::CreateComputePipeline(ComputePipelineDesc const& desc)
{
    return {};
}

MeshPipelineHandle PhxEngine::RHI::CreateMeshPipeline(MeshPipelineDesc const& desc)
{
    return {};
}

BufferHandle PhxEngine::RHI::CreateGpuBuffer(BufferDesc const& desc, void* initalData = nullptr)
{
    return {};
}

TextureHandle PhxEngine::RHI::CreateTexture(TextureDesc const& desc, void* initalData = nullptr)
{
    return {};
}