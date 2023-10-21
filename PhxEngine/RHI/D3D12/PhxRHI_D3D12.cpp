#include <PhxEngine.h>

#include "D3D12Context.h"
#include "Core/String.h"
#include <stdexcept>


using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;
// -- D3D12 Implementation

namespace
{

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

		return false;
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
			return false;
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

	
	D3D12Context::Initialize(selectedAdapter);


    return true;
}

bool PhxEngine::RHI::Finalize()
{
	D3D12Context::Finalize();
    return false;
}

void PhxEngine::RHI::RunGarbageCollection()
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

// Resource Creation Functions
template<typename T>
bool PhxEngine::RHI::CreateCommandSignature(CommandSignatureDesc const& desc, CommandSignature& out)
{
	static_assert(sizeof(T) % sizeof(uint32_t) == 0);
	return this->CreateCommandSignature(desc, sizeof(T), out);
}
bool PhxEngine::RHI::CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride, CommandSignature& out)
{
    return false;
}

bool PhxEngine::RHI::CreateSwapChain(SwapchainDesc desc, SwapChain& out)
{
	return false;
}

bool PhxEngine::RHI::CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode, Shader& out)
{
    return false;
}

bool PhxEngine::RHI::CreateInputLayout(InputLayoutDesc const& desc, uint32_t attributeCount, InputLayout& out)
{
    return false;
}

bool PhxEngine::RHI::CreateGfxPipeline(GfxPipelineDesc const& desc, Texture& out)
{
    return false;
}

bool PhxEngine::RHI::CreateComputePipeline(ComputePipelineDesc const& desc, Texture& out)
{
    return false;
}

bool PhxEngine::RHI::CreateMeshPipeline(MeshPipelineDesc const& desc, Texture& out)
{
    return false;
}

bool PhxEngine::RHI::CreateGpuBuffer(GpuBufferDesc const& desc, Texture& out, void* initalData = nullptr)
{
    return false;
}

bool PhxEngine::RHI::CreateTexture(TextureDesc const& desc, Texture& out, void* initalData = nullptr)
{
    return false;
}