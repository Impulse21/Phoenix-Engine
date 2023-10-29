#define NOMINMAX
#include <RHI/PhxRHI.h>

#include <Core/Log.h>

#include <Core/Memory.h>
#include "D3D12Device.h"
#include "D3D12GpuMemoryAllocator.h"
#include "D3D12Resources.h"
#include "D3D12ResourceManager.h"

#include "Core/String.h"
#include <stdexcept>

using namespace PhxEngine;
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

void PhxEngine::RHI::EndFrame(Core::Span<SwapChainHandle> swapchainsToPresent)
{
}

CommandList* PhxEngine::RHI::BeginCommandList(RHI::CommandListType type)
{
	return nullptr;
}

uint64_t PhxEngine::RHI::SubmitCommands(CommandList* commandList)
{
	return 0;
}
CommandSignatureHandle PhxEngine::RHI::CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride)
{
	return ResourceManager->CreateCommandSignature(desc, byteStride);
}

SwapChainHandle PhxEngine::RHI::CreateSwapChain(SwapchainDesc const& desc)
{
	return ResourceManager->CreateSwapChain(desc);
}

ShaderHandle PhxEngine::RHI::CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode)
{
	return ResourceManager->CreateShader(desc, shaderByteCode);
}

InputLayoutHandle PhxEngine::RHI::CreateInputLayout(Core::Span<VertexAttributeDesc> descs)
{
	return ResourceManager->CreateInputLayout(descs);
}

GfxPipelineHandle PhxEngine::RHI::CreateGfxPipeline(GfxPipelineDesc const& desc)
{
	return ResourceManager->CreateGfxPipeline(desc);
}

ComputePipelineHandle PhxEngine::RHI::CreateComputePipeline(ComputePipelineDesc const& desc)
{
	return ResourceManager->CreateComputePipeline(desc);
}

MeshPipelineHandle PhxEngine::RHI::CreateMeshPipeline(MeshPipelineDesc const& desc)
{
	return ResourceManager->CreateMeshPipeline(desc);
}

BufferHandle PhxEngine::RHI::CreateGpuBuffer(BufferDesc const& desc, void* initalData)
{
	return ResourceManager->CreateGpuBuffer(desc, initalData);
}

TextureHandle PhxEngine::RHI::CreateTexture(TextureDesc const& desc, void* initalData)
{
	return ResourceManager->CreateTexture(desc, initalData);
}

RenderPassHandle PhxEngine::RHI::CreateRenderPass(RenderPassDesc desc)
{
	return {};
}

void PhxEngine::RHI::DeleteCommandSignature(CommandSignatureHandle handle)
{
	ResourceManager->DeleteCommandSignature(handle);
}

void PhxEngine::RHI::DeleteSwapChain(SwapChainHandle handle)
{
	ResourceManager->DeleteSwapChain(handle);
}

void PhxEngine::RHI::DeleteShader(ShaderHandle handle)
{
	ResourceManager->DeleteShader(handle);
}

void PhxEngine::RHI::DeleteInputLayout(InputLayoutHandle handle)
{
	ResourceManager->DeleteInputLayout(handle);
}

void PhxEngine::RHI::DeleteGfxPipeline(GfxPipelineHandle handle)
{
	ResourceManager->DeleteGfxPipeline(handle);
}

void PhxEngine::RHI::DeleteComputePipeline(ComputePipelineHandle handle)
{
	ResourceManager->DeleteComputePipeline(handle);
}

void PhxEngine::RHI::DeleteMeshPipeline(MeshPipelineHandle handle)
{
	ResourceManager->DeleteMeshPipeline(handle);
}

void PhxEngine::RHI::DeleteGpuBuffer(BufferHandle handle)
{
	ResourceManager->DeleteGpuBuffer(handle);
}

void PhxEngine::RHI::DeleteTexture(TextureHandle handle)
{
	ResourceManager->DeleteTexture(handle);
}

void PhxEngine::RHI::DeleteRTAccelerationStructure(RTAccelerationStructureHandle handle)
{
	ResourceManager->DeleteRTAccelerationStructure(handle);
}

void PhxEngine::RHI::DeleteTimerQuery(TimerQueryHandle handle)
{
	ResourceManager->DeleteTimerQuery(handle);
}

void PhxEngine::RHI::ResizeSwapChain(SwapChainHandle handle, SwapchainDesc const& desc)
{
	ResourceManager->ResizeSwapChain(handle, desc);
}

RHI::Format PhxEngine::RHI::GetSwapChainFormat(SwapChainHandle handle)
{
	D3D12SwapChain* swapChain = ResourceManager->GetSwapChainPool().Get(handle);

	if (!swapChain)
	{
		return RHI::Format::UNKNOWN;
	}

	return swapChain->Desc.Format;
}

const TextureDesc& PhxEngine::RHI::GetTextureDesc(TextureHandle handle)
{
	return ResourceManager->GetTextureDesc(handle);
}

const BufferDesc& PhxEngine::RHI::GetBufferDesc(BufferHandle handle)
{
	return ResourceManager->GetBufferDesc(handle);
}

DescriptorIndex PhxEngine::RHI::GetDescriptorIndex(TextureHandle handle, SubresouceType type, int subResource)
{
	return ResourceManager->GetDescriptorIndex(handle, type, subResource);
}

DescriptorIndex PhxEngine::RHI::GetDescriptorIndex(BufferHandle handle, SubresouceType type, int subResource)
{
	return ResourceManager->GetDescriptorIndex(handle, type, subResource);
}
