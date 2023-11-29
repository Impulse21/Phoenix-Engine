#define NOMINMAX

#include "D3D12DynamicRHI.h"
#include <PhxEngine/Core/Vector.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/String.h>
#include <PhxEngine/Core/Math.h>
#include "D3D12Common.h"

#include <variant>

#include "D3D12BiindlessDescriptorTable.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <pix3.h>
#ifdef _DEBUG
#pragma comment(lib, "dxguid.lib")
#endif

#undef MemoryBarrier

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

static bool sDebugEnabled = false;

// DirectX Aligily SDK
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 608; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

// Come up with a better reporting system for resources
#define TRACK_RESOURCES 0
namespace
{

	// Define a global function to handle the messages
	void CALLBACK DebugCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext)
	{
		switch (Severity)
		{
		case D3D12_MESSAGE_SEVERITY_MESSAGE:
		case D3D12_MESSAGE_SEVERITY_INFO:
			LOG_RHI_INFO(pDescription);
			break;
		case D3D12_MESSAGE_SEVERITY_WARNING:
			LOG_RHI_WARN(pDescription);
			break;
		case D3D12_MESSAGE_SEVERITY_ERROR:
		case D3D12_MESSAGE_SEVERITY_CORRUPTION:
			LOG_RHI_ERROR(pDescription);
			break;
		}
	}

	constexpr D3D12_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
	{
		D3D12_SUBRESOURCE_DATA data = {};
		data.pData = pInitialData.pData;
		data.RowPitch = pInitialData.rowPitch;
		data.SlicePitch = pInitialData.slicePitch;

		return data;
	}

	DXGI_FORMAT ConvertFormat(RHI::Format format)
	{
		return D3D12::GetDxgiFormatMapping(format).SrvFormat;
	}

	D3D12_SHADER_VISIBILITY ConvertShaderStage(ShaderStage s)
	{
		switch (s)  // NOLINT(clang-diagnostic-switch-enum)
		{
		case ShaderStage::Vertex:
			return D3D12_SHADER_VISIBILITY_VERTEX;
		case ShaderStage::Hull:
			return D3D12_SHADER_VISIBILITY_HULL;
		case ShaderStage::Domain:
			return D3D12_SHADER_VISIBILITY_DOMAIN;
		case ShaderStage::Geometry:
			return D3D12_SHADER_VISIBILITY_GEOMETRY;
		case ShaderStage::Pixel:
			return D3D12_SHADER_VISIBILITY_PIXEL;
		case ShaderStage::Amplification:
			return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
		case ShaderStage::Mesh:
			return D3D12_SHADER_VISIBILITY_MESH;

		default:
			// catch-all case - actually some of the bitfield combinations are unrepresentable in DX12
			return D3D12_SHADER_VISIBILITY_ALL;
		}
	}

	D3D12_BLEND ConvertBlendValue(BlendFactor value)
	{
		switch (value)
		{
		case BlendFactor::Zero:
			return D3D12_BLEND_ZERO;
		case BlendFactor::One:
			return D3D12_BLEND_ONE;
		case BlendFactor::SrcColor:
			return D3D12_BLEND_SRC_COLOR;
		case BlendFactor::InvSrcColor:
			return D3D12_BLEND_INV_SRC_COLOR;
		case BlendFactor::SrcAlpha:
			return D3D12_BLEND_SRC_ALPHA;
		case BlendFactor::InvSrcAlpha:
			return D3D12_BLEND_INV_SRC_ALPHA;
		case BlendFactor::DstAlpha:
			return D3D12_BLEND_DEST_ALPHA;
		case BlendFactor::InvDstAlpha:
			return D3D12_BLEND_INV_DEST_ALPHA;
		case BlendFactor::DstColor:
			return D3D12_BLEND_DEST_COLOR;
		case BlendFactor::InvDstColor:
			return D3D12_BLEND_INV_DEST_COLOR;
		case BlendFactor::SrcAlphaSaturate:
			return D3D12_BLEND_SRC_ALPHA_SAT;
		case BlendFactor::ConstantColor:
			return D3D12_BLEND_BLEND_FACTOR;
		case BlendFactor::InvConstantColor:
			return D3D12_BLEND_INV_BLEND_FACTOR;
		case BlendFactor::Src1Color:
			return D3D12_BLEND_SRC1_COLOR;
		case BlendFactor::InvSrc1Color:
			return D3D12_BLEND_INV_SRC1_COLOR;
		case BlendFactor::Src1Alpha:
			return D3D12_BLEND_SRC1_ALPHA;
		case BlendFactor::InvSrc1Alpha:
			return D3D12_BLEND_INV_SRC1_ALPHA;
		default:
			return D3D12_BLEND_ZERO;
		}
	}

	D3D12_BLEND_OP ConvertBlendOp(EBlendOp value)
	{
		switch (value)
		{
		case EBlendOp::Add:
			return D3D12_BLEND_OP_ADD;
		case EBlendOp::Subrtact:
			return D3D12_BLEND_OP_SUBTRACT;
		case EBlendOp::ReverseSubtract:
			return D3D12_BLEND_OP_REV_SUBTRACT;
		case EBlendOp::Min:
			return D3D12_BLEND_OP_MIN;
		case EBlendOp::Max:
			return D3D12_BLEND_OP_MAX;
		default:
			return D3D12_BLEND_OP_ADD;
		}
	}

	D3D12_STENCIL_OP ConvertStencilOp(StencilOp value)
	{
		switch (value)
		{
		case StencilOp::Keep:
			return D3D12_STENCIL_OP_KEEP;
		case StencilOp::Zero:
			return D3D12_STENCIL_OP_ZERO;
		case StencilOp::Replace:
			return D3D12_STENCIL_OP_REPLACE;
		case StencilOp::IncrementAndClamp:
			return D3D12_STENCIL_OP_INCR_SAT;
		case StencilOp::DecrementAndClamp:
			return D3D12_STENCIL_OP_DECR_SAT;
		case StencilOp::Invert:
			return D3D12_STENCIL_OP_INVERT;
		case StencilOp::IncrementAndWrap:
			return D3D12_STENCIL_OP_INCR;
		case StencilOp::DecrementAndWrap:
			return D3D12_STENCIL_OP_DECR;
		default:
			return D3D12_STENCIL_OP_KEEP;
		}
	}

	D3D12_COMPARISON_FUNC ConvertComparisonFunc(ComparisonFunc value)
	{
		switch (value)
		{
		case ComparisonFunc::Never:
			return D3D12_COMPARISON_FUNC_NEVER;
		case ComparisonFunc::Less:
			return D3D12_COMPARISON_FUNC_LESS;
		case ComparisonFunc::Equal:
			return D3D12_COMPARISON_FUNC_EQUAL;
		case ComparisonFunc::LessOrEqual:
			return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case ComparisonFunc::Greater:
			return D3D12_COMPARISON_FUNC_GREATER;
		case ComparisonFunc::NotEqual:
			return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case ComparisonFunc::GreaterOrEqual:
			return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case ComparisonFunc::Always:
			return D3D12_COMPARISON_FUNC_ALWAYS;
		default:
			return D3D12_COMPARISON_FUNC_NEVER;
		}
	}
	D3D_PRIMITIVE_TOPOLOGY ConvertPrimitiveType(PrimitiveType pt, uint32_t controlPoints)
	{
		switch (pt)
		{
		case PrimitiveType::PointList:
			return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case PrimitiveType::LineList:
			return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		case PrimitiveType::TriangleList:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case PrimitiveType::TriangleStrip:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case PrimitiveType::TriangleFan:
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		case PrimitiveType::TriangleListWithAdjacency:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
		case PrimitiveType::TriangleStripWithAdjacency:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
		case PrimitiveType::PatchList:
			if (controlPoints == 0 || controlPoints > 32)
			{
				return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
			}
			return D3D_PRIMITIVE_TOPOLOGY(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (controlPoints - 1));
		default:
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}

	D3D12_TEXTURE_ADDRESS_MODE ConvertSamplerAddressMode(SamplerAddressMode mode)
	{
		switch (mode)
		{
		case SamplerAddressMode::Clamp:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case SamplerAddressMode::Wrap:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case SamplerAddressMode::Border:
			return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		case SamplerAddressMode::Mirror:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		case SamplerAddressMode::MirrorOnce:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		default:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		}
	}

	UINT ConvertSamplerReductionType(SamplerReductionType reductionType)
	{
		switch (reductionType)
		{
		case SamplerReductionType::Standard:
			return D3D12_FILTER_REDUCTION_TYPE_STANDARD;
		case SamplerReductionType::Comparison:
			return D3D12_FILTER_REDUCTION_TYPE_COMPARISON;
		case SamplerReductionType::Minimum:
			return D3D12_FILTER_REDUCTION_TYPE_MINIMUM;
		case SamplerReductionType::Maximum:
			return D3D12_FILTER_REDUCTION_TYPE_MAXIMUM;
		default:
			return D3D12_FILTER_REDUCTION_TYPE_STANDARD;
		}
	}

	constexpr DSTORAGE_REQUEST_DESTINATION_TYPE ConvertStorageDestinationType(RequestDesntination const& dest)
	{
		switch (dest)
		{
		case RequestDesntination::Buffer:
			return DSTORAGE_REQUEST_DESTINATION_BUFFER;
		case RequestDesntination::TextureRegion:
			return DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION;
		case RequestDesntination::TextureAllSubResources:
			return DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES;
		default:
			return DSTORAGE_REQUEST_DESTINATION_BUFFER;
		}
	}

	Microsoft::WRL::ComPtr<IDXGIFactory6> CreateDXGIFactory6()
	{
		uint32_t flags = 0;
		bool isDebugEnabled = IsDebuggerPresent();

		if (isDebugEnabled)
		{
			Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
			ThrowIfFailed(
				D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

			debugController->EnableDebugLayer();
			flags = DXGI_CREATE_FACTORY_DEBUG;

			Microsoft::WRL::ComPtr<ID3D12Debug3> debugController3;
			if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController3))))
			{
				debugController3->SetEnableGPUBasedValidation(false);
			}

			Microsoft::WRL::ComPtr<ID3D12Debug5> debugController5;
			if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController5))))
			{
				debugController5->SetEnableAutoName(true);
			}
		}

		Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
		ThrowIfFailed(
			CreateDXGIFactory2(flags, IID_PPV_ARGS(factory.ReleaseAndGetAddressOf())));

		return factory;
	}

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


DirectStorage::DirectStorage(ID3D12Device* device)
{
	ThrowIfFailed(
		DStorageGetFactory(IID_PPV_ARGS(this->m_factory.ReleaseAndGetAddressOf())));

	// buffer on the GPU.
	DSTORAGE_QUEUE_DESC queueDesc{};
	queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
	queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
	queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
	queueDesc.Device = device;

	ThrowIfFailed(
		this->m_factory->CreateQueue(&queueDesc, IID_PPV_ARGS(this->m_queue[DSTORAGE_REQUEST_SOURCE_FILE].ReleaseAndGetAddressOf())));


	queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
	ThrowIfFailed(
		this->m_factory->CreateQueue(&queueDesc, IID_PPV_ARGS(this->m_queue[DSTORAGE_REQUEST_SOURCE_MEMORY].ReleaseAndGetAddressOf())));

	queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
	ThrowIfFailed(
		this->m_factory->CreateQueue(&queueDesc, IID_PPV_ARGS(this->m_queue[DSTORAGE_REQUEST_SOURCE_MEMORY].ReleaseAndGetAddressOf())));

	ThrowIfFailed(
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_fence)));
}

void DirectStorage::EnqueueRequest(StorageRequest const& request)
{
	DSTORAGE_REQUEST r = request.InternalRequest;

	if (r.Options.DestinationType == DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES && request.DestinationTexture)
	{
		r.Destination.MultipleSubresources.Resource = D3D12DynamicRHI::ResourceCast(request.DestinationTexture)->D3D12Resource.Get();
		r.Destination.MultipleSubresources.FirstSubresource = 0;
	}

	this->m_queue[r.Options.SourceType]->EnqueueRequest(&r);
}

SubmitReceipt DirectStorage::Submit(bool waitForComplete)
{
	this->m_queue[DSTORAGE_REQUEST_SOURCE_MEMORY]->EnqueueSignal(this->m_fence.Get(), ++this->m_fenceValue);
	this->m_queue[DSTORAGE_REQUEST_SOURCE_MEMORY]->Submit();

	if (waitForComplete)
	{
		// Wait on the frames last value?
		// NULL event handle will simply wait immediately:
		//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion#remarks
		ThrowIfFailed(
			this->m_fence->SetEventOnCompletion(this->m_fenceValue, nullptr));
	}

	return {};
}


PhxEngine::RHI::D3D12::D3D12DynamicRHI::D3D12DynamicRHI()
	: m_timerQueryIndexPool(kTimestampQueryHeapSize)
{
	assert(D3D12DynamicRHI::SingleD3D12RHI == nullptr);
	D3D12DynamicRHI::SingleD3D12RHI = this;
}

PhxEngine::RHI::D3D12::D3D12DynamicRHI::~D3D12DynamicRHI()
{
	D3D12DynamicRHI::SingleD3D12RHI = nullptr;
}

void PhxEngine::RHI::D3D12::D3D12DynamicRHI::Initialize()
{
	LOG_RHI_INFO("Initialize DirectX 12 Graphics Device");
	this->InitializeD3D12NativeResources(this->m_gpuAdapter.NativeAdapter.Get());

#if ENABLE_PIX_CAPUTRE
	this->m_pixCaptureModule = PIXLoadLatestWinPixGpuCapturerLibrary();
#endif 

	// Create Queues
	this->m_commandQueues[(int)CommandQueueType::Graphics].Initialize(this->GetD3D12Device().Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	this->m_commandQueues[(int)CommandQueueType::Compute].Initialize(this->GetD3D12Device().Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
	this->m_commandQueues[(int)CommandQueueType::Copy].Initialize(this->GetD3D12Device().Get(), D3D12_COMMAND_LIST_TYPE_COPY);

	// -- Create Common indirect signatures --
	// Create common indirect command signatures:
	D3D12_INDIRECT_ARGUMENT_DESC dispatchArgs[1];
	dispatchArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

	D3D12_COMMAND_SIGNATURE_DESC cmdDesc = {};
	cmdDesc.ByteStride = sizeof(IndirectDispatchArgs);
	cmdDesc.NumArgumentDescs = 1;
	cmdDesc.pArgumentDescs = dispatchArgs;
	ThrowIfFailed(
		this->GetD3D12Device()->CreateCommandSignature(&cmdDesc, nullptr, IID_PPV_ARGS(&this->m_dispatchIndirectCommandSignature)));


	D3D12_INDIRECT_ARGUMENT_DESC drawInstancedArgs[1];
	drawInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

	cmdDesc.ByteStride = sizeof(IndirectDrawArgInstanced);
	cmdDesc.NumArgumentDescs = 1;
	cmdDesc.pArgumentDescs = drawInstancedArgs;
	ThrowIfFailed(
		this->GetD3D12Device()->CreateCommandSignature(&cmdDesc, nullptr, IID_PPV_ARGS(&this->m_drawInstancedIndirectCommandSignature)));

	D3D12_INDIRECT_ARGUMENT_DESC drawIndexedInstancedArgs[1];
	drawIndexedInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

	cmdDesc.ByteStride = sizeof(IndirectDrawArgsIndexedInstanced);
	cmdDesc.NumArgumentDescs = 1;
	cmdDesc.pArgumentDescs = drawIndexedInstancedArgs;
	ThrowIfFailed(
		this->GetD3D12Device()->CreateCommandSignature(&cmdDesc, nullptr, IID_PPV_ARGS(&this->m_drawIndexedInstancedIndirectCommandSignature)));

	if (this->CheckCapability(DeviceCapability::MeshShading))
	{
		D3D12_INDIRECT_ARGUMENT_DESC dispatchMeshArgs[1];
		dispatchMeshArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;

		cmdDesc.ByteStride = sizeof(IndirectDispatchArgs);
		cmdDesc.NumArgumentDescs = 1;
		cmdDesc.pArgumentDescs = dispatchMeshArgs;
		ThrowIfFailed(
			this->GetD3D12Device()->CreateCommandSignature(&cmdDesc, nullptr, IID_PPV_ARGS(&this->m_dispatchMeshIndirectCommandSignature)));
	}

	// Create Descriptor Heaps
	this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
		this,
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].Initialize(
		this,
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::RTV].Initialize(
		this,
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::DSV].Initialize(
		this,
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


	this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
		this,
		NUM_BINDLESS_RESOURCES,
		TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE - NUM_BINDLESS_RESOURCES,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);


	this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].Initialize(
		this,
		10,
		100,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);


	this->m_bindlessResourceDescriptorTable.Initialize(this->GetResourceGpuHeap().Allocate(NUM_BINDLESS_RESOURCES));

	D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
	allocatorDesc.pDevice = this->m_rootDevice.Get();
	allocatorDesc.pAdapter = this->m_gpuAdapter.NativeAdapter.Get();
	//allocatorDesc.PreferredBlockSize = 256 * 1024 * 1024;
	//allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
	allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS)(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);

	ThrowIfFailed(
		D3D12MA::CreateAllocator(&allocatorDesc, &this->m_d3d12MemAllocator));

#if 0
	// TODO: Data drive device create info
	this->CreateGpuTimestampQueryHeap(this->kTimestampQueryHeapSize);

	// Create GPU Buffer for timestamp readbacks
	this->m_timestampQueryBuffer = this->CreateBuffer({
		.Usage = Usage::ReadBack,
		.Stride = sizeof(uint64_t),
		.NumElements = this->kTimestampQueryHeapSize,
		.DebugName = "Timestamp Query Buffer" });
#endif
	for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
	{
		ThrowIfFailed(
			this->GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_frameFences[q])));
	}

	this->m_directStorage = Core::RefCountPtr<DirectStorage>::Create(phx_new(DirectStorage(this->m_rootDevice.Get())));
}

void PhxEngine::RHI::D3D12::D3D12DynamicRHI::Finalize()
{
	this->WaitForIdle();

#if 0
	this->DeleteBuffer(this->m_timestampQueryBuffer);

	for (auto handle : impl->BackBuffers)
	{
		this->m_texturePool.Release(handle);
	}

	impl->BackBuffers.clear();

	for (int i = 0; i < this->m_frameCommandListHandles.size(); i++)
	{
		this->m_commandListPool.Release(this->m_frameCommandListHandles[i]);
	}
#endif


	this->RunGarbageCollection(~0u);

	RefCountPtr<ID3D12InfoQueue1> infoQueue;
	if (SUCCEEDED(this->m_rootDevice->QueryInterface<ID3D12InfoQueue1>(&infoQueue)))
	{
		infoQueue->UnregisterMessageCallback(this->m_CallbackCookie);
	}

#if ENABLE_PIX_CAPUTRE
	FreeLibrary(this->m_pixCaptureModule);
#endif
}
void D3D12DynamicRHI::Wait(SubmitReceipt const& reciet)
{
}

void PhxEngine::RHI::D3D12::D3D12DynamicRHI::Present(ISwapChain* swapChain)
{
	// -- Mark Queues for completion ---
	for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
	{
		D3D12CommandQueue& queue = this->m_commandQueues[q];
		// Single Frame Fence
		queue.GetD3D12CommandQueue()->Signal(this->m_frameFences[q].Get(), this->m_frameCount);
	}

	// -- Present SwapChain ---

	D3D12SwapChain* impl = ResourceCast(swapChain);
	{
		UINT presentFlags = 0;
		if (!impl->GetDesc().VSync && !impl->GetDesc().Fullscreen)
		{
			presentFlags = DXGI_PRESENT_ALLOW_TEARING;
		}
		HRESULT hr = impl->NativeSwapchain4->Present((UINT)impl->GetDesc().VSync, presentFlags);

		// If the device was reset we must completely reinitialize the renderer.
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{

#ifdef _DEBUG
			char buff[64] = {};
			sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
				static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED)
					? this->GetD3D12Device()->GetDeviceRemovedReason()
					: hr));
			OutputDebugStringA(buff);
#endif
		}
	}
	// Begin the next frame - this affects the GetCurrentBackBufferIndex()
	this->m_frameCount++;

	// Wait for next frame, run this in a worker thread.
	this->RunGarbageCollection(this->m_frameCount);
	{
		for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
		{
			const UINT64 completedFrame = this->m_frameFences[q]->GetCompletedValue();

			// If the fence is max uint64, that might indicate that the device has been removed as fences get reset in this case
			assert(completedFrame != UINT64_MAX);
			const uint32_t bufferCount = MaxFramesInflgiht;

			// Since our frame count is 1 based rather then 0, increment the number of buffers by 1 so we don't have to wait on the first 3 frames
			// that are kicked off.
			if (this->m_frameCount >= (bufferCount + 1) && completedFrame < this->m_frameCount)
			{
				// Wait on the frames last value?
				// NULL event handle will simply wait immediately:
				//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion#remarks
				ThrowIfFailed(
					this->m_frameFences[q]->SetEventOnCompletion(this->m_frameCount - bufferCount, nullptr));
			}
		}
	}
}
SubmitReceipt D3D12DynamicRHI::ExecuteCommandLists(Core::Span<ICommandList*> commandLists, CommandQueueType executionQueue)
{
	D3D12CommandQueue& queue = this->GetQueue(executionQueue);

	static thread_local Phx::FlexArray<ID3D12CommandList*> d3d12CommandLists;
	d3d12CommandLists.clear();

	for (ICommandList* commandList : commandLists)
	{
		auto impl = ResourceCast(commandList);
		assert(impl);
		impl->NativeCommandList->Close();
		d3d12CommandLists.push_back(impl->NativeCommandList.Get());
	}

	auto fenceValue = queue.ExecuteCommandLists(d3d12CommandLists);


	for (auto commandList : commandLists)
	{
		auto cmdImpl = ResourceCast(commandList);
		queue.DiscardAllocator(fenceValue, cmdImpl->NativeCommandAllocator);
		cmdImpl->NativeCommandAllocator = nullptr;
	}

	bool waitForCompletion = false;
	if (waitForCompletion)
	{
		queue.WaitForFence(fenceValue);

#if 0
		for (auto commandList : commandLists)
		{
			auto cmdImpl = ResourceCast(commandList);

			for (auto timerQueryHandle : cmdImpl->GetTimerQueries())
			{
				auto timerQuery = this->GetTimerQueryPool().Get(timerQueryHandle);
				if (timerQuery)
				{
					timerQuery->Started = true;
					timerQuery->Resolved = false;
				}
			}
		}
#endif
	}
	else
	{
#if 0
		for (auto commandList : commandLists)
		{
			auto cmdImpl = SafeCast<D3D12CommandList*>(commandList);

			for (auto timerQueryHandle : cmdImpl->GetTimerQueries())
			{
				auto timerQuery = this->GetTimerQueryPool().Get(timerQueryHandle);
				if (timerQuery)
				{
					timerQuery->Started = true;
					timerQuery->Resolved = false;
					timerQuery->CommandQueue = queue;
					timerQuery->FenceCount = fenceValue;
				}
			}
		}
#endif
	}

	return { fenceValue, queue.GetFence()};
}

void PhxEngine::RHI::D3D12::D3D12DynamicRHI::WaitForIdle()
{
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	HRESULT hr = this->GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
	{
		D3D12CommandQueue& queue = this->m_commandQueues[q];
		queue.WaitForIdle();
	}
}

bool PhxEngine::RHI::D3D12::D3D12DynamicRHI::IsHdrSwapchainSupported(D3D12SwapChain* swapChain)
{
	if (!swapChain->NativeSwapchain)
	{
		return false;
	}

	// HDR display query: https://docs.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
	Microsoft::WRL::ComPtr<IDXGIOutput> dxgiOutput;
	if (SUCCEEDED(swapChain->NativeSwapchain->GetContainingOutput(&dxgiOutput)))
	{
		Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
		if (SUCCEEDED(dxgiOutput.As(&output6)))
		{
			DXGI_OUTPUT_DESC1 desc1;
			if (SUCCEEDED(output6->GetDesc1(&desc1)))
			{
				if (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
				{
					return true;
				}
			}
		}
	}

	return false;
}

int PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateSubresource(D3D12Texture* texture, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	switch (subresourceType)
	{
	case SubresouceType::SRV:
		return this->CreateShaderResourceView(texture, firstSlice, sliceCount, firstMip, mipCount);
	case SubresouceType::UAV:
		return this->CreateUnorderedAccessView(texture, firstSlice, sliceCount, firstMip, mipCount);
	case SubresouceType::RTV:
		return this->CreateRenderTargetView(texture, firstSlice, sliceCount, firstMip, mipCount);
	case SubresouceType::DSV:
		return this->CreateDepthStencilView(texture, firstSlice, sliceCount, firstMip, mipCount);
	default:
		throw std::runtime_error("Unknown sub resource type");
	}
}

int PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateSubresource(D3D12Buffer* buffer, SubresouceType subresourceType, size_t offset, size_t size)
{
	switch (subresourceType)
	{
	case SubresouceType::SRV:
		return this->CreateShaderResourceView(buffer, offset, size);
	case SubresouceType::UAV:
		return this->CreateUnorderedAccessView(buffer, offset, size);
	default:
		throw std::runtime_error("Unknown sub resource type");
	}
}

int PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateShaderResourceView(D3D12Buffer* buffer, size_t offset, size_t size)
{
	D3D12Buffer* bufferImpl = buffer;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	const uint32_t stideInBytes = bufferImpl->GetDesc().SizeInBytes;
	if (bufferImpl->GetDesc().Format == RHI::Format::UNKNOWN)
	{
		if ((bufferImpl->GetDesc().MiscFlags & BufferMiscFlags::Raw) == BufferMiscFlags::Raw)
		{
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			srvDesc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
			srvDesc.Buffer.NumElements = (UINT)std::min(size, stideInBytes - offset) / sizeof(uint32_t);
		}
		else if ((bufferImpl->GetDesc().MiscFlags & BufferMiscFlags::Structured) == BufferMiscFlags::Structured)
		{
			uint32_t strideInBytes = (bufferImpl->GetDesc().SizeInBytes);
			// This is a Structured Buffer
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.FirstElement = (UINT)offset / strideInBytes;
			srvDesc.Buffer.NumElements = (UINT)std::min(size, stideInBytes - offset) / strideInBytes;
			srvDesc.Buffer.StructureByteStride = strideInBytes;
		}
	}
	else
	{
		throw std::runtime_error("Unsupported at this time.");
#if false
		uint32_t stride = GetFormatStride(format);
		srv_desc.Format = _ConvertFormat(format);
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.FirstElement = offset / stride;
		srv_desc.Buffer.NumElements = (UINT)std::min(size, desc.size - offset) / stride;
#endif
	}

	DescriptorView view = {
			.Allocation = this->GetResourceCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.SRVDesc = srvDesc,
	};

	this->GetD3D12Device2()->CreateShaderResourceView(
		bufferImpl->D3D12Resource.Get(),
		&srvDesc,
		view.Allocation.GetCpuHandle());

	const bool isBindless = (bufferImpl->GetDesc().MiscFlags & RHI::BufferMiscFlags::Bindless) == RHI::BufferMiscFlags::Bindless;
	if (isBindless)
	{
		// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
		view.BindlessIndex = this->m_bindlessResourceDescriptorTable.Allocate();
		if (view.BindlessIndex != cInvalidDescriptorIndex)
		{
			this->GetD3D12Device2()->CopyDescriptorsSimple(
				1,
				this->m_bindlessResourceDescriptorTable.GetCpuHandle(view.BindlessIndex),
				view.Allocation.GetCpuHandle(),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}

	if (bufferImpl->Srv.Allocation.IsNull())
	{
		bufferImpl->Srv = view;
		return -1;
	}

	bufferImpl->SrvSubresourcesAlloc.push_back(view);
	return bufferImpl->SrvSubresourcesAlloc.size() - 1;
}

int PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateUnorderedAccessView(D3D12Buffer* buffer, size_t offset, size_t size)
{
	D3D12Buffer* bufferImpl = buffer;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

	const uint32_t stideInBytes = bufferImpl->GetDesc().SizeInBytes;
	const bool hasCounter = (bufferImpl->GetDesc().MiscFlags & BufferMiscFlags::HasCounter) == BufferMiscFlags::HasCounter;
	if (bufferImpl->GetDesc().Format == RHI::Format::UNKNOWN)
	{
		if ((bufferImpl->GetDesc().MiscFlags & BufferMiscFlags::Raw) == BufferMiscFlags::Raw)
		{
			uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
			uavDesc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
			uavDesc.Buffer.NumElements = (UINT)std::min(size, stideInBytes - offset) / sizeof(uint32_t);
		}
		else if ((bufferImpl->GetDesc().MiscFlags & BufferMiscFlags::Structured) == BufferMiscFlags::Structured)
		{
			uint32_t strideInBytes = (bufferImpl->GetDesc().SizeInBytes);
			// This is a Structured Buffer
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.Buffer.FirstElement = (UINT)offset / strideInBytes;
			uavDesc.Buffer.NumElements = (UINT)std::min(size, stideInBytes - offset) / strideInBytes;
			uavDesc.Buffer.StructureByteStride = strideInBytes;

			if (hasCounter)
			{
				assert(false);
				// uavDesc.Buffer.CounterOffsetInBytes = bufferImpl->GetDesc().UavCounterOffset;
			}
		}
	}
	else
	{
		throw std::runtime_error("Unsupported at this time.");
#if false
		uint32_t stride = GetFormatStride(format);
		uavDesc.Format = _ConvertFormat(format);
		uavDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = offset / stride;
		uavDesc.Buffer.NumElements = (UINT)std::min(size, desc.size - offset) / stride;
#endif
	}

	DescriptorView view = {
			.Allocation = this->GetResourceCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.UAVDesc = uavDesc,
	};

	ID3D12Resource* counterResource = nullptr;
	if (hasCounter)
	{
		assert(false);
#if 0
		if (bufferImpl->GetDesc().UavCounterBuffer.IsValid())
		{
			D3D12Buffer* counterBuffer = this->m_bufferPool.Get(bufferImpl->GetDesc().UavCounterBuffer);
			counterResource = counterBuffer->D3D12Resource.Get();

		}
		else
		{
			counterResource = bufferImpl->D3D12Resource.Get();
		}
#endif
	}

	this->GetD3D12Device2()->CreateUnorderedAccessView(
		bufferImpl->D3D12Resource.Get(),
		counterResource,
		&uavDesc,
		view.Allocation.GetCpuHandle());

	const bool isBindless = (bufferImpl->GetDesc().MiscFlags & RHI::BufferMiscFlags::Bindless) == RHI::BufferMiscFlags::Bindless;
	if (isBindless)
	{
		// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
		view.BindlessIndex = this->m_bindlessResourceDescriptorTable.Allocate();
		if (view.BindlessIndex != cInvalidDescriptorIndex)
		{
			this->GetD3D12Device2()->CopyDescriptorsSimple(
				1,
				this->m_bindlessResourceDescriptorTable.GetCpuHandle(view.BindlessIndex),
				view.Allocation.GetCpuHandle(),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}

	if (bufferImpl->UavAllocation.Allocation.IsNull())
	{
		bufferImpl->UavAllocation = view;

		return -1;
	}

	bufferImpl->UavSubresourcesAlloc.push_back(view);
	return bufferImpl->UavSubresourcesAlloc.size() - 1;
}

int PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateShaderResourceView(D3D12Texture* texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	D3D12Texture* textureImpl = texture;

	auto dxgiFormatMapping = GetDxgiFormatMapping(textureImpl->GetDesc().Format);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = dxgiFormatMapping.SrvFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	uint32_t planeSlice = (srvDesc.Format == DXGI_FORMAT_X24_TYPELESS_G8_UINT) ? 1 : 0;

	if (textureImpl->GetDesc().Dimension == TextureDimension::TextureCube)
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;  // Only 2D textures are supported (this was checked in the calling function).
		srvDesc.TextureCube.MipLevels = mipCount;
		srvDesc.TextureCube.MostDetailedMip = firstMip;
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;  // Only 2D textures are supported (this was checked in the calling function).
		srvDesc.Texture2D.MipLevels = mipCount;
	}
	switch (textureImpl->GetDesc().Dimension)
	{
	case TextureDimension::Texture1D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		srvDesc.Texture1D.MostDetailedMip = firstMip; // Subresource data
		srvDesc.Texture1D.MipLevels = mipCount;// Subresource data
		break;
	case TextureDimension::Texture1DArray:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		srvDesc.Texture1DArray.FirstArraySlice = firstSlice;
		srvDesc.Texture1DArray.ArraySize = sliceCount;// Subresource data
		srvDesc.Texture1DArray.MostDetailedMip = firstMip;// Subresource data
		srvDesc.Texture1DArray.MipLevels = mipCount;// Subresource data
		break;
	case TextureDimension::Texture2D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = firstMip;// Subresource data
		srvDesc.Texture2D.MipLevels = mipCount;// Subresource data
		srvDesc.Texture2D.PlaneSlice = planeSlice;
		break;
	case TextureDimension::Texture2DArray:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.FirstArraySlice = firstSlice;// Subresource data
		srvDesc.Texture2DArray.ArraySize = sliceCount;// Subresource data
		srvDesc.Texture2DArray.MostDetailedMip = firstMip;// Subresource data
		srvDesc.Texture2DArray.MipLevels = mipCount;// Subresource data
		srvDesc.Texture2DArray.PlaneSlice = planeSlice;
		break;
	case TextureDimension::TextureCube:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = firstMip;// Subresource data
		srvDesc.TextureCube.MipLevels = mipCount;// Subresource data
		break;
	case TextureDimension::TextureCubeArray:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
		srvDesc.TextureCubeArray.First2DArrayFace = 0;// Subresource data
		srvDesc.TextureCubeArray.NumCubes = textureImpl->GetDesc().ArraySize / 6;// Subresource data
		srvDesc.TextureCubeArray.MostDetailedMip = firstMip;// Subresource data
		srvDesc.TextureCubeArray.MipLevels = mipCount;// Subresource data
		break;
	case TextureDimension::Texture2DMS:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		break;
	case TextureDimension::Texture2DMSArray:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
		srvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;// Subresource data
		srvDesc.Texture2DMSArray.ArraySize = sliceCount;// Subresource data
		break;
	case TextureDimension::Texture3D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MostDetailedMip = firstMip;// Subresource data
		srvDesc.Texture3D.MipLevels = mipCount;// Subresource data
		break;
	case TextureDimension::Unknown:
	default:
		throw std::runtime_error("Unsupported Enum");
	}

	DescriptorView view = {
			.Allocation = this->GetResourceCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.SRVDesc = srvDesc,
			.FirstMip = firstMip,
			.MipCount = mipCount,
			.FirstSlice = firstSlice,
			.SliceCount = sliceCount
	};

	this->GetD3D12Device2()->CreateShaderResourceView(
		textureImpl->D3D12Resource.Get(),
		&srvDesc,
		view.Allocation.GetCpuHandle());

	if ((textureImpl->GetDesc().MiscFlags | TextureMiscFlags::Bindless) == TextureMiscFlags::Bindless)
	{
		// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
		view.BindlessIndex = this->m_bindlessResourceDescriptorTable.Allocate();
		if (view.BindlessIndex != cInvalidDescriptorIndex)
		{
			this->GetD3D12Device2()->CopyDescriptorsSimple(
				1,
				this->m_bindlessResourceDescriptorTable.GetCpuHandle(view.BindlessIndex),
				view.Allocation.GetCpuHandle(),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}

	if (textureImpl->Srv.Allocation.IsNull())
	{
		textureImpl->Srv = view;
		return -1;
	}

	textureImpl->SrvSubresourcesAlloc.push_back(view);
	return textureImpl->SrvSubresourcesAlloc.size() - 1;
}

int PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateRenderTargetView(D3D12Texture* texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	D3D12Texture* textureImpl = texture;

	auto dxgiFormatMapping = GetDxgiFormatMapping(textureImpl->GetDesc().Format);
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = dxgiFormatMapping.RtvFormat;

	switch (textureImpl->GetDesc().Dimension)
	{
	case TextureDimension::Texture1D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		rtvDesc.Texture1D.MipSlice = firstMip;
		break;
	case TextureDimension::Texture1DArray:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		rtvDesc.Texture1DArray.FirstArraySlice = firstSlice;
		rtvDesc.Texture1DArray.ArraySize = sliceCount;
		rtvDesc.Texture1DArray.MipSlice = firstMip;
		break;
	case TextureDimension::Texture2D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = firstMip;
		break;
	case TextureDimension::Texture2DArray:
	case TextureDimension::TextureCube:
	case TextureDimension::TextureCubeArray:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.ArraySize = sliceCount;
		rtvDesc.Texture2DArray.FirstArraySlice = firstSlice;
		rtvDesc.Texture2DArray.MipSlice = firstMip;
		break;
	case TextureDimension::Texture2DMS:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		break;
	case TextureDimension::Texture2DMSArray:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		rtvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
		rtvDesc.Texture2DMSArray.ArraySize = sliceCount;
		break;
	case TextureDimension::Texture3D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		rtvDesc.Texture3D.FirstWSlice = 0;
		rtvDesc.Texture3D.WSize = textureImpl->GetDesc().ArraySize;
		rtvDesc.Texture3D.MipSlice = firstMip;
		break;
	case TextureDimension::Unknown:
	default:
		throw std::runtime_error("Unsupported Enum");
	}

	DescriptorView view = {
			.Allocation = this->GetRtvCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			.RTVDesc = rtvDesc,
			.FirstMip = firstMip,
			.MipCount = mipCount,
			.FirstSlice = firstSlice,
			.SliceCount = sliceCount
	};

	this->GetD3D12Device2()->CreateRenderTargetView(
		textureImpl->D3D12Resource.Get(),
		&rtvDesc,
		view.Allocation.GetCpuHandle());

	if (textureImpl->RtvAllocation.Allocation.IsNull())
	{
		textureImpl->RtvAllocation = view;
		return -1;
	}

	textureImpl->RtvSubresourcesAlloc.push_back(view);
	return textureImpl->RtvSubresourcesAlloc.size() - 1;
}

int PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateDepthStencilView(D3D12Texture* texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	D3D12Texture* textureImpl = texture;

	auto dxgiFormatMapping = GetDxgiFormatMapping(textureImpl->GetDesc().Format);
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = dxgiFormatMapping.RtvFormat;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	switch (textureImpl->GetDesc().Dimension)
	{
	case TextureDimension::Texture1D:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
		dsvDesc.Texture1D.MipSlice = firstMip;
		break;
	case TextureDimension::Texture1DArray:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		dsvDesc.Texture1DArray.FirstArraySlice = firstSlice;
		dsvDesc.Texture1DArray.ArraySize = sliceCount;
		dsvDesc.Texture1DArray.MipSlice = firstMip;
		break;
	case TextureDimension::Texture2D:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = firstMip;
		break;
	case TextureDimension::Texture2DArray:
	case TextureDimension::TextureCube:
	case TextureDimension::TextureCubeArray:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.ArraySize = sliceCount;
		dsvDesc.Texture2DArray.FirstArraySlice = firstSlice;
		dsvDesc.Texture2DArray.MipSlice = firstMip;
		break;
	case TextureDimension::Texture2DMS:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
		break;
	case TextureDimension::Texture2DMSArray:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		dsvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
		dsvDesc.Texture2DMSArray.ArraySize = sliceCount;
		break;
	case TextureDimension::Texture3D:
	{
		throw std::runtime_error("Unsupported Dimension");
	}
	case TextureDimension::Unknown:
	default:
		throw std::runtime_error("Unsupported Enum");
	}

	DescriptorView view = {
			.Allocation = this->GetDsvCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			.DSVDesc = dsvDesc,
			.FirstMip = firstMip,
			.MipCount = mipCount,
			.FirstSlice = firstSlice,
			.SliceCount = sliceCount
	};

	this->GetD3D12Device2()->CreateDepthStencilView(
		textureImpl->D3D12Resource.Get(),
		&dsvDesc,
		view.Allocation.GetCpuHandle());

	if (textureImpl->DsvAllocation.Allocation.IsNull())
	{
		textureImpl->DsvAllocation = view;
		return -1;
	}

	textureImpl->DsvSubresourcesAlloc.push_back(view);
	return textureImpl->DsvSubresourcesAlloc.size() - 1;
}

int PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateUnorderedAccessView(D3D12Texture* texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	D3D12Texture* textureImpl = texture;

	auto dxgiFormatMapping = GetDxgiFormatMapping(textureImpl->GetDesc().Format);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = dxgiFormatMapping.SrvFormat;

	switch (textureImpl->GetDesc().Dimension)
	{
	case TextureDimension::Texture1D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		uavDesc.Texture1D.MipSlice = firstMip;
		break;
	case TextureDimension::Texture1DArray:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		uavDesc.Texture1DArray.FirstArraySlice = firstSlice;
		uavDesc.Texture1DArray.ArraySize = sliceCount;
		uavDesc.Texture1DArray.MipSlice = firstMip;
		break;
	case TextureDimension::Texture2D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = firstMip;
		break;
	case TextureDimension::Texture2DArray:
	case TextureDimension::TextureCube:
	case TextureDimension::TextureCubeArray:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.FirstArraySlice = firstSlice;
		uavDesc.Texture2DArray.ArraySize = sliceCount;
		uavDesc.Texture2DArray.MipSlice = firstMip;
		break;
	case TextureDimension::Texture3D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.FirstWSlice = 0;
		uavDesc.Texture3D.WSize = textureImpl->GetDesc().Depth;
		uavDesc.Texture3D.MipSlice = firstMip;
		break;
	case TextureDimension::Texture2DMS:
	case TextureDimension::Texture2DMSArray:
	{
		throw std::runtime_error("Unsupported Dimension");
	}
	case TextureDimension::Unknown:
	default:
		throw std::runtime_error("Unsupported Enum");
	}

	DescriptorView view = {
			.Allocation = this->GetResourceCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.UAVDesc = uavDesc,
			.FirstMip = firstMip,
			.MipCount = mipCount,
			.FirstSlice = firstSlice,
			.SliceCount = sliceCount
	};

	this->GetD3D12Device2()->CreateUnorderedAccessView(
		textureImpl->D3D12Resource.Get(),
		nullptr,
		&uavDesc,
		view.Allocation.GetCpuHandle());

	if ((textureImpl->GetDesc().MiscFlags | TextureMiscFlags::Bindless) == TextureMiscFlags::Bindless)
	{
		// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
		view.BindlessIndex = this->m_bindlessResourceDescriptorTable.Allocate();
		if (view.BindlessIndex != cInvalidDescriptorIndex)
		{
			this->GetD3D12Device2()->CopyDescriptorsSimple(
				1,
				this->m_bindlessResourceDescriptorTable.GetCpuHandle(view.BindlessIndex),
				view.Allocation.GetCpuHandle(),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}

	if (textureImpl->UavAllocation.Allocation.IsNull())
	{
		textureImpl->UavAllocation = view;
		return -1;
	}

	textureImpl->UavSubresourcesAlloc.push_back(view);
	return textureImpl->UavSubresourcesAlloc.size() - 1;
}

bool PhxEngine::RHI::D3D12::D3D12DynamicRHI::IsDevicedRemoved()
{
	HRESULT hr =  this->GetD3D12Device5()->GetDeviceRemovedReason();
	return FAILED(hr);
}

bool PhxEngine::RHI::D3D12::D3D12DynamicRHI::CheckCapability(DeviceCapability deviceCapability)
{
	return EnumHasAllFlags(this->m_capabilities, deviceCapability);
}

SwapChainRef PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateSwapChain(SwapChainDesc const& desc, void* windowHandle)
{
	HRESULT hr;
	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	const DxgiFormatMapping& formatMapping = GetDxgiFormatMapping(desc.Format);

	// Create swapchain:
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = desc.Width;
	swapChainDesc.Height = desc.Height;
	swapChainDesc.Format = formatMapping.RtvFormat;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = MaxFramesInflgiht;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = swapChainFlags;

	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
	fullscreenDesc.Windowed = !desc.Fullscreen;


	SwapChainRef swapChain = SwapChainRef::Create(phx_new(D3D12SwapChain));
	D3D12SwapChain* d3D12SwapChain = ResourceCast(swapChain.Get());

	hr = this->GetDxgiFactory()->CreateSwapChainForHwnd(
		this->GetQueue(RHI::CommandQueueType::Graphics).GetD3D12CommandQueue(),
		static_cast<HWND>(windowHandle),
		&swapChainDesc,
		&fullscreenDesc,
		nullptr,
		d3D12SwapChain->NativeSwapchain.GetAddressOf()
	);

	ThrowIfFailed(hr);
	hr = d3D12SwapChain->NativeSwapchain->QueryInterface(IID_PPV_ARGS(&d3D12SwapChain->NativeSwapchain4));
	ThrowIfFailed(hr);

	this->CreateSwapChainResources(d3D12SwapChain);

	return swapChain;
}

ShaderRef PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode)
{
	ShaderRef retVal = ShaderRef::Create(phx_new(D3D12Shader(desc, shaderByteCode.begin(), shaderByteCode.Size())));
	D3D12Shader* shaderImpl = ResourceCast(retVal.Get());
	shaderImpl->Desc = desc;

	auto hr = D3D12CreateVersionedRootSignatureDeserializer(
		shaderImpl->ByteCode.data(),
		shaderImpl->ByteCode.size(),
		IID_PPV_ARGS(&shaderImpl->RootSignatureDeserializer));

	if (SUCCEEDED(hr))
	{
		hr = shaderImpl->RootSignatureDeserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1, &shaderImpl->RootSignatureDesc);
		if (SUCCEEDED(hr))
		{
			assert(shaderImpl->RootSignatureDesc->Version == D3D_ROOT_SIGNATURE_VERSION_1_1);

			Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
			hr = this->m_rootDevice->CreateRootSignature(
				0,
				shaderImpl->ByteCode.data(),
				shaderImpl->ByteCode.size(),
				IID_PPV_ARGS(&rootSig)
			);
			assert(SUCCEEDED(hr));
			if (SUCCEEDED(hr))
			{
				shaderImpl->RootSignature = rootSig;
			}
		}
	}

	return retVal;
}

InputLayoutRef PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount)
{
	InputLayoutRef retVal = InputLayoutRef::Create(phx_new(D3D12InputLayout));
	D3D12InputLayout* inputLayoutImpl = ResourceCast(retVal.Get());

	inputLayoutImpl->Attributes.resize(attributeCount);

	for (uint32_t index = 0; index < attributeCount; index++)
	{
		VertexAttributeDesc& attr = inputLayoutImpl->Attributes[index];

		// Copy the description to get a stable name pointer in desc
		attr = desc[index];

		D3D12_INPUT_ELEMENT_DESC& dx12Desc = inputLayoutImpl->InputElements.emplace_back();

		dx12Desc.SemanticName = attr.SemanticName.c_str();
		dx12Desc.SemanticIndex = attr.SemanticIndex;


		const DxgiFormatMapping& formatMapping = GetDxgiFormatMapping(attr.Format);
		dx12Desc.Format = formatMapping.SrvFormat;
		dx12Desc.InputSlot = attr.InputSlot;
		dx12Desc.AlignedByteOffset = attr.AlignedByteOffset;
		if (dx12Desc.AlignedByteOffset == VertexAttributeDesc::SAppendAlignedElement)
		{
			dx12Desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		}

		if (attr.IsInstanced)
		{
			dx12Desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
			dx12Desc.InstanceDataStepRate = 1;
		}
		else
		{
			dx12Desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			dx12Desc.InstanceDataStepRate = 0;
		}
	}

	return retVal;
}

GfxPipelineRef PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateGfxPipeline(GfxPipelineDesc const& desc)
{
	GfxPipelineRef retVal = GfxPipelineRef::Create(phx_new(D3D12GfxPipeline));
	D3D12GfxPipeline* pipeline = ResourceCast(retVal.Get());
	pipeline->Desc = desc;

	if (desc.VertexShader)
	{
		D3D12Shader* shaderImpl = ResourceCast(desc.VertexShader.Get());
		if (pipeline->RootSignature == nullptr)
		{
			pipeline->RootSignature = shaderImpl->RootSignature;
		}
	}

	if (desc.PixelShader)
	{
		D3D12Shader* shaderImpl = ResourceCast(desc.PixelShader.Get());
		if (pipeline->RootSignature == nullptr)
		{
			pipeline->RootSignature = shaderImpl->RootSignature;
		}
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12Desc = {};
	d3d12Desc.pRootSignature = pipeline->RootSignature.Get();

	D3D12Shader* shaderImpl = nullptr;
	shaderImpl = ResourceCast(desc.VertexShader.Get());
	if (shaderImpl)
	{
		d3d12Desc.VS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = ResourceCast(desc.HullShader.Get());
	if (shaderImpl)
	{
		d3d12Desc.HS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = ResourceCast(desc.DomainShader.Get());
	if (shaderImpl)
	{
		d3d12Desc.DS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = ResourceCast(desc.GeometryShader.Get());
	if (shaderImpl)
	{
		d3d12Desc.GS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}
	;
	shaderImpl = ResourceCast(desc.PixelShader.Get());
	if (shaderImpl)
	{
		d3d12Desc.PS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	this->TranslateBlendState(desc.BlendRenderState, d3d12Desc.BlendState);
	this->TranslateDepthStencilState(desc.DepthStencilRenderState, d3d12Desc.DepthStencilState);
	this->TranslateRasterState(desc.RasterRenderState, d3d12Desc.RasterizerState);

	switch (desc.PrimType)
	{
	case PrimitiveType::PointList:
		d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case PrimitiveType::LineList:
		d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case PrimitiveType::TriangleList:
	case PrimitiveType::TriangleStrip:
		d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case PrimitiveType::PatchList:
		d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		break;
	}

	if (desc.DsvFormat.has_value())
	{
		d3d12Desc.DSVFormat = GetDxgiFormatMapping(desc.DsvFormat.value()).RtvFormat;
	}

	d3d12Desc.SampleDesc.Count = desc.SampleCount;
	d3d12Desc.SampleDesc.Quality = desc.SampleQuality;

	for (size_t i = 0; i < desc.RtvFormats.size(); i++)
	{
		d3d12Desc.RTVFormats[i] = GetDxgiFormatMapping(desc.RtvFormats[i]).RtvFormat;
	}

	D3D12InputLayout* inputLayout = ResourceCast(desc.InputLayout.Get());
	if (inputLayout && !inputLayout->InputElements.empty())
	{
		d3d12Desc.InputLayout.NumElements = uint32_t(inputLayout->InputElements.size());
		d3d12Desc.InputLayout.pInputElementDescs = &(inputLayout->InputElements[0]);
	}

	d3d12Desc.NumRenderTargets = (uint32_t)desc.RtvFormats.size();
	d3d12Desc.SampleMask = ~0u;

	ThrowIfFailed(
		this->GetD3D12Device2()->CreateGraphicsPipelineState(&d3d12Desc, IID_PPV_ARGS(&pipeline->D3D12PipelineState)));

	return retVal;
}

ComputePipelineRef PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateComputePipeline(ComputePipelineDesc const& desc)
{
	ComputePipelineRef retVal = ComputePipelineRef::Create(phx_new(D3D12ComputePipeline));
	D3D12ComputePipeline* pipeline = ResourceCast(retVal.Get());
	pipeline->Desc = desc;

	assert(desc.ComputeShader);
	D3D12Shader* shaderImpl = ResourceCast(desc.ComputeShader.Get());
	if (pipeline->RootSignature == nullptr)
	{
		pipeline->RootSignature = shaderImpl->RootSignature;
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12Desc = {};
	d3d12Desc.pRootSignature = pipeline->RootSignature.Get();
	d3d12Desc.CS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };

	ThrowIfFailed(
		this->GetD3D12Device2()->CreateComputePipelineState(&d3d12Desc, IID_PPV_ARGS(&pipeline->D3D12PipelineState)));

	return retVal;
}

MeshPipelineRef PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateMeshPipeline(MeshPipelineDesc const& desc)
{
	MeshPipelineRef retVal = MeshPipelineRef::Create(phx_new(D3D12MeshPipeline));
	D3D12MeshPipeline* pipeline = ResourceCast(retVal.Get());
	pipeline->Desc = desc;

	if (desc.AmpShader)
	{
		D3D12Shader* shaderImpl = ResourceCast(desc.AmpShader.Get());
		if (pipeline->RootSignature == nullptr)
		{
			pipeline->RootSignature = shaderImpl->RootSignature;
		}
	}

	if (desc.MeshShader)
	{
		D3D12Shader* shaderImpl = ResourceCast(desc.MeshShader.Get());
		if (pipeline->RootSignature == nullptr)
		{
			pipeline->RootSignature = shaderImpl->RootSignature;
		}
	}

	if (desc.PixelShader)
	{
		D3D12Shader* shaderImpl = ResourceCast(desc.PixelShader.Get());
		if (pipeline->RootSignature == nullptr)
		{
			pipeline->RootSignature = shaderImpl->RootSignature;
		}
	}

#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
	struct PSO_STREAM
	{
		typedef __declspec(align(sizeof(void*))) D3D12_PIPELINE_STATE_SUBOBJECT_TYPE ALIGNED_TYPE;

		ALIGNED_TYPE RootSignature_Type;        ID3D12RootSignature* RootSignature;
		ALIGNED_TYPE PrimitiveTopology_Type;    D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
		ALIGNED_TYPE AmplificationShader_Type;  D3D12_SHADER_BYTECODE AmplificationShader;
		ALIGNED_TYPE MeshShader_Type;           D3D12_SHADER_BYTECODE MeshShader;
		ALIGNED_TYPE PixelShader_Type;          D3D12_SHADER_BYTECODE PixelShader;
		ALIGNED_TYPE RasterizerState_Type;      D3D12_RASTERIZER_DESC RasterizerState;
		ALIGNED_TYPE DepthStencilState_Type;    D3D12_DEPTH_STENCIL_DESC DepthStencilState;
		ALIGNED_TYPE BlendState_Type;           D3D12_BLEND_DESC BlendState;
		ALIGNED_TYPE SampleDesc_Type;           DXGI_SAMPLE_DESC SampleDesc;
		ALIGNED_TYPE SampleMask_Type;           UINT SampleMask;
		ALIGNED_TYPE RenderTargets_Type;        D3D12_RT_FORMAT_ARRAY RenderTargets;
		ALIGNED_TYPE DSVFormat_Type;            DXGI_FORMAT DSVFormat;
	} psoDesc = { };
#pragma warning(pop)

	psoDesc.RootSignature_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
	psoDesc.PrimitiveTopology_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
	psoDesc.AmplificationShader_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS;
	psoDesc.MeshShader_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS;
	psoDesc.PixelShader_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
	psoDesc.RasterizerState_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
	psoDesc.DepthStencilState_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
	psoDesc.BlendState_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
	psoDesc.SampleDesc_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC;
	psoDesc.SampleMask_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK;
	psoDesc.RenderTargets_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
	psoDesc.DSVFormat_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;

	psoDesc.RootSignature = pipeline->RootSignature.Get();

	D3D12Shader* shaderImpl = nullptr;
	shaderImpl = ResourceCast(desc.AmpShader.Get());
	if (shaderImpl)
	{
		psoDesc.AmplificationShader = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = ResourceCast(desc.MeshShader.Get());
	if (shaderImpl)
	{
		psoDesc.MeshShader = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = ResourceCast(desc.PixelShader.Get());
	if (shaderImpl)
	{
		psoDesc.PixelShader = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	this->TranslateBlendState(desc.BlendRenderState, psoDesc.BlendState);
	this->TranslateDepthStencilState(desc.DepthStencilRenderState, psoDesc.DepthStencilState);
	this->TranslateRasterState(desc.RasterRenderState, psoDesc.RasterizerState);

	switch (desc.PrimType)
	{
	case PrimitiveType::PointList:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case PrimitiveType::LineList:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case PrimitiveType::TriangleList:
	case PrimitiveType::TriangleStrip:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case PrimitiveType::PatchList:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		break;
	}
	psoDesc.SampleDesc.Count = desc.SampleCount;
	psoDesc.SampleDesc.Quality = desc.SampleQuality;
	psoDesc.SampleMask = ~0u;

	psoDesc.RenderTargets.NumRenderTargets = desc.RtvFormats.size();

	for (size_t i = 0; i < desc.RtvFormats.size(); i++)
	{
		psoDesc.RenderTargets.RTFormats[i] = GetDxgiFormatMapping(desc.RtvFormats[i]).RtvFormat;
	}

	if (desc.DsvFormat.has_value())
	{
		psoDesc.DSVFormat = GetDxgiFormatMapping(desc.DsvFormat.value()).RtvFormat;
	}

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
	streamDesc.pPipelineStateSubobjectStream = &psoDesc;
	streamDesc.SizeInBytes = sizeof(psoDesc);

	ThrowIfFailed(
		this->GetD3D12Device2()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipeline->D3D12PipelineState)));

	return retVal;
}

TextureRef PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateTexture(TextureDesc const& desc)
{
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


	TextureRef retVal = TextureRef::Create(phx_new(D3D12Texture));
	D3D12Texture* textureImpl = ResourceCast(retVal.Get());
	textureImpl->Desc = desc;

	textureImpl->TotalSize = 0;
	textureImpl->Footprints.resize(desc.ArraySize * std::max(uint16_t(1u), desc.MipLevels));
	textureImpl->RowSizesInBytes.resize(textureImpl->Footprints.size());
	textureImpl->NumRows.resize(textureImpl->Footprints.size());
	this->GetD3D12Device2()->GetCopyableFootprints(
		&resourceDesc,
		0,
		(UINT)textureImpl->Footprints.size(),
		0,
		textureImpl->Footprints.data(),
		textureImpl->NumRows.data(),
		textureImpl->RowSizesInBytes.data(),
		&textureImpl->TotalSize
	);

	ThrowIfFailed(
		this->m_d3d12MemAllocator->CreateResource(
			&allocationDesc,
			&resourceDesc,
			ConvertResourceStates(desc.InitialState),
			useClearValue ? &d3d12OptimizedClearValue : nullptr,
			&textureImpl->Allocation,
			IID_PPV_ARGS(&textureImpl->D3D12Resource)));


	std::wstring debugName;
	Core::StringConvert(desc.DebugName, debugName);
	textureImpl->D3D12Resource->SetName(debugName.c_str());
	textureImpl->Allocation->SetName(debugName.c_str());

	if ((desc.BindingFlags & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		this->CreateSubresource(textureImpl, RHI::SubresouceType::SRV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
	{
		this->CreateSubresource(textureImpl, RHI::SubresouceType::RTV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
	{
		this->CreateSubresource(textureImpl, RHI::SubresouceType::DSV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		this->CreateSubresource(textureImpl, RHI::SubresouceType::UAV, 0, ~0u, 0, ~0u);
	}

	return retVal;
}

BufferRef PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateBuffer(BufferDesc const& desc)
{
	BufferRef retVal = BufferRef::Create(phx_new(D3D12Buffer));
	D3D12Buffer* bufferImpl = ResourceCast(retVal.Get());
	bufferImpl->Desc = desc;

	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
	if ((desc.Binding & RHI::BindingFlags::UnorderedAccess) == RHI::BindingFlags::UnorderedAccess)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	switch (desc.Usage)
	{
	case Usage::ReadBack:
		allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;

		initialState = D3D12_RESOURCE_STATE_COPY_DEST;
		break;

	case Usage::Upload:
		allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
		break;

	case Usage::Default:
	default:
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		initialState = D3D12_RESOURCE_STATE_COMMON;
	}

	UINT64 alignedSize = 0;
	if ((desc.Binding & BindingFlags::ConstantBuffer) == BindingFlags::ConstantBuffer)
	{
		alignedSize = Core::AlignTo(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.SizeInBytes, resourceFlags, alignedSize);

	/* TODO: Add Sparse stuff
	if (resource_heap_tier >= D3D12_RESOURCE_HEAP_TIER_2)
	{
		// tile pool memory can be used for sparse buffers and textures alike (requires resource heap tier 2):
		allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
	}
	else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_BUFFER))
	{
		allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	}
	else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_NON_RT_DS))
	{
		allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
	}
	else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_RT_DS))
	{
		allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
	}
	*/

	if ((desc.MiscFlags & BufferMiscFlags::IsAliasedResource) == BufferMiscFlags::IsAliasedResource)
	{
		D3D12_RESOURCE_ALLOCATION_INFO finalAllocInfo = {};
		finalAllocInfo.Alignment = 0;
		finalAllocInfo.SizeInBytes = Core::AlignTo(
			desc.SizeInBytes,
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * 1024);

		ThrowIfFailed(
			this->m_d3d12MemAllocator->AllocateMemory(
				&allocationDesc,
				&finalAllocInfo,
				&bufferImpl->Allocation));
		return retVal;
	}
#if 0
	else if (desc.AliasedBuffer.IsValid())
	{
		D3D12Buffer* aliasedBuffer = this->m_bufferPool.Get(desc.AliasedBuffer);

		ThrowIfFailed(
			this->m_d3d12MemAllocator->CreateAliasingResource(
				aliasedBuffer->Allocation.Get(),
				0,
				&resourceDesc,
				initialState,
				nullptr,
				IID_PPV_ARGS(&bufferImpl->D3D12Resource)));
	}
#endif
	else
	{
		ThrowIfFailed(
			this->m_d3d12MemAllocator->CreateResource(
				&allocationDesc,
				&resourceDesc,
				initialState,
				nullptr,
				&bufferImpl->Allocation,
				IID_PPV_ARGS(&bufferImpl->D3D12Resource)));
	}

	switch (desc.Usage)
	{
	case Usage::ReadBack:
	{
		ThrowIfFailed(
			bufferImpl->D3D12Resource->Map(0, nullptr, &bufferImpl->MappedData));
		bufferImpl->MappedSizeInBytes = static_cast<uint32_t>(desc.SizeInBytes);

		break;
	}

	case Usage::Upload:
	{
		D3D12_RANGE readRange = {};
		ThrowIfFailed(
			bufferImpl->D3D12Resource->Map(0, &readRange, &bufferImpl->MappedData));

		bufferImpl->MappedSizeInBytes = static_cast<uint32_t>(desc.SizeInBytes);
		break;
	}
	case Usage::Default:
	default:
		break;
	}

	std::wstring debugName(desc.DebugName.begin(), desc.DebugName.end());
	bufferImpl->D3D12Resource->SetName(debugName.c_str());
	bufferImpl->Allocation->SetName(debugName.c_str());

	if ((desc.MiscFlags & BufferMiscFlags::IsAliasedResource) == BufferMiscFlags::IsAliasedResource)
	{
		return retVal;
	}

	if ((desc.Binding & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		this->CreateSubresource(bufferImpl, RHI::SubresouceType::SRV, 0u);
	}

	if ((desc.Binding & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		this->CreateSubresource(bufferImpl, RHI::SubresouceType::UAV, 0u);
	}

	return retVal;
}

CommandListRef PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateCommandList(RHI::CommandQueueType type)
{
	CommandListRef retVal = CommandListRef::Create(phx_new(D3D12CommandList));
	D3D12CommandList* commandList = ResourceCast(retVal.Get());

	commandList->QueueType = type;
	commandList->NativeCommandAllocator = this->GetQueue(type).RequestAllocator();

	this->GetD3D12Device()->CreateCommandList(
		0,
		this->GetQueue(type).GetType(),
		commandList->NativeCommandAllocator,
		nullptr,
		IID_PPV_ARGS(&commandList->NativeCommandList));

	commandList->NativeCommandList->SetName(L"D3D12GfxDevice::CommandList");
	ThrowIfFailed(
		commandList->NativeCommandList->QueryInterface<ID3D12GraphicsCommandList6>(&commandList->NativeCommandList6));

	commandList->UploadBuffer = std::make_unique<D3D12::UploadBuffer>();
	commandList->UploadBuffer->Initialize();

	commandList->NativeCommandList->Close();

	return retVal;
}

void PhxEngine::RHI::D3D12::D3D12DynamicRHI::ResizeSwapChain(ISwapChain* swapChain, SwapChainDesc const& desc)
{
	if (!swapChain)
	{
		return;
	}

	WaitForIdle();
	D3D12SwapChain* impl = ResourceCast(swapChain);
	
	// We need to release resources prior to calling resize
	impl->BackBuffers.clear();
	impl->Desc = desc;

	this->RunGarbageCollection(~0u);

	const auto& formatMapping = GetDxgiFormatMapping(desc.Format);
	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ThrowIfFailed(
		impl->NativeSwapchain4->ResizeBuffers(
			desc.BufferCount,
			desc.Width,
			desc.Height,
			formatMapping.RtvFormat,
			swapChainFlags));

	this->CreateSwapChainResources(impl);
}

void PhxEngine::RHI::D3D12::D3D12DynamicRHI::RunGarbageCollection(uint64_t completedFrame)
{
	// TODO: Do this on a worker thread.
	while (!this->m_deleteQueue.empty())
	{
		DeleteItem& deleteItem = this->m_deleteQueue.front();
		if (deleteItem.Frame < completedFrame)
		{
			deleteItem.DeleteFn();
			this->m_deleteQueue.pop_front();
		}
		else
		{
			break;
		}
	}
}

void PhxEngine::RHI::D3D12::D3D12DynamicRHI::TranslateBlendState(BlendRenderState const& inState, D3D12_BLEND_DESC& outState)
{
	outState.AlphaToCoverageEnable = inState.alphaToCoverageEnable;
	outState.IndependentBlendEnable = true;

	for (uint32_t i = 0; i < cMaxRenderTargets; i++)
	{
		const auto& src = inState.Targets[i];
		auto& dst = outState.RenderTarget[i];


		dst.BlendEnable = src.BlendEnable ? TRUE : FALSE;
		dst.SrcBlend = ConvertBlendValue(src.SrcBlend);
		dst.DestBlend = ConvertBlendValue(src.DestBlend);
		dst.BlendOp = ConvertBlendOp(src.BlendOp);
		dst.SrcBlendAlpha = ConvertBlendValue(src.SrcBlendAlpha);
		dst.DestBlendAlpha = ConvertBlendValue(src.DestBlendAlpha);
		dst.BlendOpAlpha = ConvertBlendOp(src.BlendOpAlpha);
		dst.RenderTargetWriteMask = (D3D12_COLOR_WRITE_ENABLE)src.ColorWriteMask;
	}
}

void PhxEngine::RHI::D3D12::D3D12DynamicRHI::TranslateDepthStencilState(DepthStencilRenderState const& inState, D3D12_DEPTH_STENCIL_DESC& outState)
{
	outState.DepthEnable = inState.DepthTestEnable ? TRUE : FALSE;
	outState.DepthWriteMask = inState.DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	outState.DepthFunc = ConvertComparisonFunc(inState.DepthFunc);
	outState.StencilEnable = inState.StencilEnable ? TRUE : FALSE;
	outState.StencilReadMask = (UINT8)inState.StencilReadMask;
	outState.StencilWriteMask = (UINT8)inState.StencilWriteMask;
	outState.FrontFace.StencilFailOp = ConvertStencilOp(inState.FrontFaceStencil.FailOp);
	outState.FrontFace.StencilDepthFailOp = ConvertStencilOp(inState.FrontFaceStencil.DepthFailOp);
	outState.FrontFace.StencilPassOp = ConvertStencilOp(inState.FrontFaceStencil.PassOp);
	outState.FrontFace.StencilFunc = ConvertComparisonFunc(inState.FrontFaceStencil.StencilFunc);
	outState.BackFace.StencilFailOp = ConvertStencilOp(inState.BackFaceStencil.FailOp);
	outState.BackFace.StencilDepthFailOp = ConvertStencilOp(inState.BackFaceStencil.DepthFailOp);
	outState.BackFace.StencilPassOp = ConvertStencilOp(inState.BackFaceStencil.PassOp);
	outState.BackFace.StencilFunc = ConvertComparisonFunc(inState.BackFaceStencil.StencilFunc);
}

void PhxEngine::RHI::D3D12::D3D12DynamicRHI::TranslateRasterState(RasterRenderState const& inState, D3D12_RASTERIZER_DESC& outState)
{
	switch (inState.FillMode)
	{
	case RasterFillMode::Solid:
		outState.FillMode = D3D12_FILL_MODE_SOLID;
		break;
	case RasterFillMode::Wireframe:
		outState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		break;
	default:
		break;
	}

	switch (inState.CullMode)
	{
	case RasterCullMode::Back:
		outState.CullMode = D3D12_CULL_MODE_BACK;
		break;
	case RasterCullMode::Front:
		outState.CullMode = D3D12_CULL_MODE_FRONT;
		break;
	case RasterCullMode::None:
		outState.CullMode = D3D12_CULL_MODE_NONE;
		break;
	default:
		break;
	}

	outState.FrontCounterClockwise = inState.FrontCounterClockwise ? TRUE : FALSE;
	outState.DepthBias = inState.DepthBias;
	outState.DepthBiasClamp = inState.DepthBiasClamp;
	outState.SlopeScaledDepthBias = inState.SlopeScaledDepthBias;
	outState.DepthClipEnable = inState.DepthClipEnable ? TRUE : FALSE;
	outState.MultisampleEnable = inState.MultisampleEnable ? TRUE : FALSE;
	outState.AntialiasedLineEnable = inState.AntialiasedLineEnable ? TRUE : FALSE;
	outState.ConservativeRaster = inState.ConservativeRasterEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	outState.ForcedSampleCount = inState.ForcedSampleCount;
}

void D3D12DynamicRHI::CreateGpuTimestampQueryHeap(uint32_t queryCount)
{
	D3D12_QUERY_HEAP_DESC dx12Desc = {};
	dx12Desc.Count = queryCount;
	dx12Desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

	ThrowIfFailed(
		this->m_rootDevice->CreateQueryHeap(
			&dx12Desc,
			IID_PPV_ARGS(&this->m_gpuTimestampQueryHeap)));
}

void PhxEngine::RHI::D3D12::D3D12DynamicRHI::CreateSwapChainResources(D3D12SwapChain* swapChain)
{
	swapChain->BackBuffers.clear();
	swapChain->BackBuffers.resize(swapChain->GetDesc().BufferCount);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = D3D12::GetDxgiFormatMapping(swapChain->GetDesc().Format).RtvFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	for (int i = 0; i < swapChain->BackBuffers.size(); i++)
	{
		RefCountPtr<ID3D12Resource> resource;
		ThrowIfFailed(
			swapChain->NativeSwapchain4->GetBuffer(i, IID_PPV_ARGS(&resource)));


		// Create a back buffer Texture
		swapChain->BackBuffers[i] = RefCountPtr<D3D12Texture>::Create(phx_new(D3D12Texture));

		swapChain->BackBuffers[i]->D3D12Resource = resource;
		DescriptorView& view = swapChain->BackBuffers[i]->RtvAllocation;
		view.Allocation = this->GetRtvCpuHeap().Allocate(1);
		view.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		view.RTVDesc = rtvDesc;

		this->GetD3D12Device()->CreateRenderTargetView(
			swapChain->BackBuffers[i]->D3D12Resource.Get(),
			&rtvDesc,
			view.Allocation.GetCpuHandle());
	}
}

void PhxEngine::RHI::D3D12::D3D12DynamicRHI::InitializeD3D12NativeResources(IDXGIAdapter* gpuAdapter)
{
	this->m_factory = CreateDXGIFactory6();
	FindAdapter(this->m_factory, this->m_gpuAdapter);

	if (!this->m_gpuAdapter.NativeAdapter)
	{
		// LOG_CORE_ERROR("Unable to create D3D12 RHI On current platform.");
	}

	ThrowIfFailed(
		D3D12CreateDevice(
			this->m_gpuAdapter.NativeAdapter.Get(),
			D3D_FEATURE_LEVEL_11_1,
			IID_PPV_ARGS(&this->m_rootDevice)));

	ThrowIfFailed(
		D3D12CreateDevice(
			gpuAdapter,
			D3D_FEATURE_LEVEL_11_1,
			IID_PPV_ARGS(&this->m_rootDevice)));

	this->m_rootDevice->SetName(L"D3D12DynamicRHI::RootDevice");
	Microsoft::WRL::ComPtr<IUnknown> renderdoc;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, RenderdocUUID, &renderdoc)))
	{
		this->IsUnderGraphicsDebugger |= !!renderdoc;
	}

	Microsoft::WRL::ComPtr<IUnknown> pix;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, PixUUID, &pix)))
	{
		this->IsUnderGraphicsDebugger |= !!pix;
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};
	bool hasOptions = SUCCEEDED(this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

	if (hasOptions)
	{
		if (featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
		{
			this->m_capabilities |= DeviceCapability::RT_VT_ArrayIndex_Without_GS;
		}
	}

	// TODO: Move to acability array
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
	bool hasOptions5 = SUCCEEDED(this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

	if (SUCCEEDED(this->m_rootDevice.As(&this->m_rootDevice5)) && hasOptions5)
	{
		if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
		{
			this->m_capabilities |= DeviceCapability::RayTracing;
		}
		if (featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0)
		{
			this->m_capabilities |= DeviceCapability::RenderPass;
		}
		if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
		{
			this->m_capabilities |= DeviceCapability::RayQuery;
		}
	}


	D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
	bool hasOptions6 = SUCCEEDED(this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

	if (hasOptions6)
	{
		if (featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
		{
			this->m_capabilities |= DeviceCapability::VariableRateShading;
		}
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
	bool hasOptions7 = SUCCEEDED(this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

	if (SUCCEEDED(this->m_rootDevice.As(&this->m_rootDevice2)) && hasOptions7)
	{
		if (featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
		{
			this->m_capabilities |= DeviceCapability::MeshShading;
		}
		this->m_capabilities |= DeviceCapability::CreateNoteZeroed;
	}

	this->FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(this->m_rootDevice2->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &this->FeatureDataRootSignature, sizeof(this->FeatureDataRootSignature))))
	{
		this->FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Check shader model support
	this->FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
	this->m_minShaderModel = ShaderModel::SM_6_6;
	if (FAILED(this->m_rootDevice2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &this->FeatureDataShaderModel, sizeof(this->FeatureDataShaderModel))))
	{
		this->FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
		this->m_minShaderModel = ShaderModel::SM_6_5;
	}


	static const bool debugEnabled = IsDebuggerPresent();
	if (debugEnabled)
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue1> infoQueue;
		if (SUCCEEDED(this->m_rootDevice->QueryInterface<ID3D12InfoQueue1>(&infoQueue)))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
			infoQueue->RegisterMessageCallback(DebugCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &this->m_CallbackCookie);

			D3D12_MESSAGE_SEVERITY severities[] =
			{
				D3D12_MESSAGE_SEVERITY_INFO,
			};

			D3D12_MESSAGE_ID denyIds[] =
			{
				// This occurs when there are uninitialized descriptors in a descriptor table, even when a
				// shader does not access the missing descriptors.  I find this is common when switching
				// shader permutations and not wanting to change much code to reorder resources.
				D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

				// Triggered when a shader does not export all color components of a render target, such as
				// when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
				D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

				// This occurs when a descriptor table is unbound even when a shader does not access the missing
				// descriptors.  This is common with a root signature shared between disparate shaders that
				// don't all need the same types of resources.
				D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

				// RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
				(D3D12_MESSAGE_ID)1008,
			};

			D3D12_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumSeverities = static_cast<UINT>(std::size(severities));
			filter.DenyList.pSeverityList = severities;
			filter.DenyList.NumIDs = static_cast<UINT>(std::size(denyIds));
			filter.DenyList.pIDList = denyIds;
			infoQueue->PushStorageFilter(&filter);
		}
	}

	// Create Compiler
	// ThrowIfFailed(
		// DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&context.dxcUtils)));

}


void PhxEngine::RHI::D3D12::D3D12DynamicRHI::FindAdapter(Microsoft::WRL::ComPtr<IDXGIFactory6> factory, D3D12Adapter& outAdapter)
{
	LOG_RHI_INFO("Finding a suitable adapter");

	// Create factory
	Microsoft::WRL::ComPtr<IDXGIAdapter1> selectedAdapter;
	D3D12DeviceBasicInfo selectedBasicDeviceInfo = {};
	size_t selectedGPUVideoMemeory = 0;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> tempAdapter;
	for (uint32_t adapterIndex = 0; D3D12Adapter::EnumAdapters(adapterIndex, factory.Get(), tempAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
	{
		if (!tempAdapter)
		{
			continue;
		}

		DXGI_ADAPTER_DESC1 desc = {};
		tempAdapter->GetDesc1(&desc);

		std::string name;
		Core::StringConvert(desc.Description, name);
		size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
		size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
		size_t sharedSystemMemory = desc.SharedSystemMemory;

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

		if (!selectedAdapter || selectedGPUVideoMemeory < dedicatedVideoMemory)
		{
			selectedAdapter = tempAdapter;
			selectedGPUVideoMemeory = dedicatedVideoMemory;
			selectedBasicDeviceInfo = basicDeviceInfo;
		}
	}

	if (!selectedAdapter)
	{
		LOG_RHI_WARN("No suitable adapters were found.");
		return;
	}

	DXGI_ADAPTER_DESC desc = {};
	selectedAdapter->GetDesc(&desc);

	std::string name;
	Core::StringConvert(desc.Description, name);
	size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
	size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
	size_t sharedSystemMemory = desc.SharedSystemMemory;

	// TODO: FIXLOG
	LOG_RHI_INFO(
		"Found Suitable D3D12 Adapter '%s'",
		name.c_str());

	// TODO: FIXLOG
	LOG_RHI_INFO(
		"Adapter has %dMB of dedicated video memory, %dMB of dedicated system memory, and %dMB of shared system memory.",
		dedicatedVideoMemory / (1024 * 1024),
		dedicatedSystemMemory / (1024 * 1024),
		sharedSystemMemory / (1024 * 1024));

	outAdapter.Name = name;
	outAdapter.BasicDeviceInfo = selectedBasicDeviceInfo;
	outAdapter.NativeDesc = desc;
	outAdapter.NativeAdapter = selectedAdapter;
}

HRESULT D3D12Adapter::EnumAdapters(uint32_t adapterIndex, IDXGIFactory6* factory6, IDXGIAdapter1** outAdapter)
{
	return factory6->EnumAdapterByGpuPreference(
		adapterIndex,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		IID_PPV_ARGS(outAdapter));
}
