#include "pch.h"

#include "D3D12Common.h"
#include "phxRhi_D3D12.h"


using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::d3d12;

using namespace Microsoft::WRL;

#ifdef USING_D3D12_AGILITY_SDK
extern "C"
{
    // Used to enable the "Agility SDK" components
    __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif

namespace
{
    static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
    static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

	// helper function for texture subresource calculations
	// https://msdn.microsoft.com/en-us/library/windows/desktop/dn705766(v=vs.85).aspx
	uint32_t CalcSubresource(uint32_t mipSlice, uint32_t arraySlice, uint32_t planeSlice, uint32_t mipLevels, uint32_t arraySize)
	{
		return mipSlice + (arraySlice * mipLevels) + (planeSlice * mipLevels * arraySize);
	}

	constexpr D3D12_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
	{
		D3D12_SUBRESOURCE_DATA data = {};
		data.pData = pInitialData.pData;
		data.RowPitch = pInitialData.rowPitch;
		data.SlicePitch = pInitialData.slicePitch;

		return data;
	}

	DXGI_FORMAT ConvertFormat(rhi::Format format)
	{
		return d3d12::GetDxgiFormatMapping(format).SrvFormat;
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

	Microsoft::WRL::ComPtr<IDXGIFactory6> CreateDXGIFactory6()
	{
		uint32_t flags = 0;
#if defined(_DEBUG)
		// Enable the debug layer (requires the Graphics Tools "optional feature").
		//
		// NOTE: Enabling the debug layer after device creation will invalidate the active device.
		{
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
			{
				debugController->EnableDebugLayer();
			}
			else
			{
				OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
			}

			ComPtr<ID3D12Debug3> debugController3;
			if (SUCCEEDED(debugController.As(&debugController3)))
			{
				debugController3->SetEnableGPUBasedValidation(false);
			}

			ComPtr<ID3D12Debug5> debugController5;
			if (SUCCEEDED(debugController.As(&debugController5)))
			{
				debugController5->SetEnableAutoName(true);
			}

			ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
			{
				flags = DXGI_CREATE_FACTORY_DEBUG;

				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

				DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
				{
					80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
				};
				DXGI_INFO_QUEUE_FILTER filter = {};
				filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
				filter.DenyList.pIDList = hide;
				dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
			}

		}
#endif

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
				PHX_CORE_ERROR("D3D12CreateDevice failed.");
			}
		}
		catch (...)
		{
		}
#pragma warning(default:6322)

		return false;
	}
}


phx::rhi::d3d12::D3D12GfxDevice::D3D12GfxDevice(InitDesc const& initDesc)
	: m_initDesc(initDesc)
{
	assert(D3D12GfxDevice::gPtr == nullptr);
	D3D12GfxDevice::gPtr == this;
}

phx::rhi::d3d12::D3D12GfxDevice::~D3D12GfxDevice()
{
	D3D12GfxDevice::gPtr = nullptr;
}

void phx::rhi::d3d12::D3D12GfxDevice::Initialize()
{
	PHX_CORE_INFO("Initialize DirectX 12 Graphics Device");
	this->InitializeResourcePools();
	this->InitializeD3D12NativeResources(this->m_gpuAdapter.NativeAdapter.Get());

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

	if (this->GetDeviceCapability().MeshShading)
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

	// TODO: Data drive device create info
	this->CreateGpuTimestampQueryHeap(this->kTimestampQueryHeapSize);

	// Create GPU Buffer for timestamp readbacks
	this->m_timestampQueryBuffer = this->CreateBuffer({
		.Usage = Usage::ReadBack,
		.Stride = sizeof(uint64_t),
		.NumElements = this->kTimestampQueryHeapSize,
		.DebugName = "Timestamp Query Buffer" });

	this->CreateSwapChain(this->m_initDesc.SwapChain, this->m_initDesc.SwapChain.Window);

	this->m_commandLists.reserve(this->m_initDesc.CommandListsPerFrame);
	for (int i = 0; i < this->m_commandLists.size(); i++)
	{
		// Create
	}

	// -- Mark Queues for completion ---
	for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
	{
		ThrowIfFailed(
			this->GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_frameFences[q])));
	}

	ID3D12Device* pDevice = this->m_rootDevice.Get();
	ID3D12CommandQueue* pCommandQueue = this->GetGfxQueue();
	// OPTICK_GPU_INIT_D3D12(pDevice, &pCommandQueue, 1);
}


void phx::rhi::d3d12::D3D12GfxDevice::Initialize()
{
	PHX_CORE_INFO("Initialize DirectX 12 Graphics Device");
	this->InitializeResourcePools();
	this->InitializeD3D12NativeResources(this->m_gpuAdapter.NativeAdapter.Get());

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

	if (this->GetDeviceCapability().MeshShading)
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

	// TODO: Data drive device create info
	this->CreateGpuTimestampQueryHeap(this->kTimestampQueryHeapSize);

	// Create GPU Buffer for timestamp readbacks
	this->m_timestampQueryBuffer = this->CreateBuffer({
		.Usage = Usage::ReadBack,
		.Stride = sizeof(uint64_t),
		.NumElements = this->kTimestampQueryHeapSize,
		.DebugName = "Timestamp Query Buffer" });

	this->CreateSwapChain(this->m_initDesc.SwapChain, this->m_initDesc.SwapChain.Window);

	this->m_commandLists.resize(this->m_initDesc.CommandListsPerFrame);

	// -- Mark Queues for completion ---
	for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
	{
		ThrowIfFailed(
			this->GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_frameFences[q])));
	}

	ID3D12Device* pDevice = this->m_rootDevice.Get();
	ID3D12CommandQueue* pCommandQueue = this->GetGfxQueue();
	// OPTICK_GPU_INIT_D3D12(pDevice, &pCommandQueue, 1);
}

void phx::rhi::d3d12::D3D12GfxDevice::Finalize()
{
	this->WaitForIdle();

	this->DeleteBuffer(this->m_timestampQueryBuffer);

	for (auto handle : this->m_swapChain.BackBuffers)
	{
		this->m_texturePool.Release(handle);
	}

	this->m_swapChain.BackBuffers.clear();

	this->RunGarbageCollection(~0u);

	this->m_texturePool.Finalize();
	this->m_commandSignaturePool.Finalize();
	this->m_shaderPool.Finalize();
	this->m_inputLayoutPool.Finalize();
	this->m_bufferPool.Finalize();
	this->m_rtAccelerationStructurePool.Finalize();
	this->m_gfxPipelinePool.Finalize();
	this->m_computePipelinePool.Finalize();
	this->m_meshPipelinePool.Finalize();

#if ENABLE_PIX_CAPUTRE
	FreeLibrary(this->m_pixCaptureModule);
#endif
}

void phx::rhi::d3d12::D3D12GfxDevice::ResizeSwapchain(SwapChainDesc const& desc)
{
	this->CreateSwapChain(desc, nullptr);
}

void phx::rhi::d3d12::D3D12GfxDevice::WaitForIdle()
{
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	HRESULT hr = this->GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
	{
		D3D12CommandQueue& queue = this->m_commandQueues[q];
		hr = queue.GetD3D12CommandQueue()->Signal(fence.Get(), 1);
		assert(SUCCEEDED(hr));
		if (fence->GetCompletedValue() < 1)
		{
			hr = fence->SetEventOnCompletion(1, NULL);
			assert(SUCCEEDED(hr));
		}
		fence->Signal(0);
	}
}

bool phx::rhi::d3d12::D3D12GfxDevice::IsHdrSwapchainSupported()
{
	if (!this->m_swapChain.NativeSwapchain)
	{
		return false;
	}

	// HDR display query: https://docs.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
	Microsoft::WRL::ComPtr<IDXGIOutput> dxgiOutput;
	if (SUCCEEDED(this->m_swapChain.NativeSwapchain->GetContainingOutput(&dxgiOutput)))
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

CommandSignatureHandle phx::rhi::d3d12::D3D12GfxDevice::CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride)
{
	CommandSignatureHandle handle = this->m_commandSignaturePool.Emplace();
	D3D12CommandSignature* signature = this->m_commandSignaturePool.Get(handle);

	signature->D3D12Descs.resize(desc.ArgDesc.Size());
	for (int i = 0; i < desc.ArgDesc.Size(); i++)
	{
		const IndirectArgumnetDesc& argDesc = desc.ArgDesc[i];
		D3D12_INDIRECT_ARGUMENT_DESC& d3d12DispatchArgs = signature->D3D12Descs[i];
		switch (argDesc.Type)
		{
		case IndirectArgumentType::Draw:
			d3d12DispatchArgs.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
			break;

		case IndirectArgumentType::DrawIndex:
			d3d12DispatchArgs.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
			break;
		case IndirectArgumentType::Dispatch:
			d3d12DispatchArgs.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
			break;
		case IndirectArgumentType::DispatchMesh:
			d3d12DispatchArgs.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
			break;
		case IndirectArgumentType::Constant:
			d3d12DispatchArgs.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
			d3d12DispatchArgs.Constant.DestOffsetIn32BitValues = argDesc.Constant.DestOffsetIn32BitValues;
			d3d12DispatchArgs.Constant.RootParameterIndex = argDesc.Constant.RootParameterIndex;
			d3d12DispatchArgs.Constant.Num32BitValuesToSet = argDesc.Constant.Num32BitValuesToSet;
			break;
		default:
			break;
		}
	}

	D3D12_COMMAND_SIGNATURE_DESC cmdDesc = {};
	cmdDesc.ByteStride = byteStride;
	cmdDesc.NumArgumentDescs = signature->D3D12Descs.size();
	cmdDesc.pArgumentDescs = signature->D3D12Descs.data();

	ID3D12RootSignature* rootSig = nullptr;
	switch (desc.PipelineType)
	{
	case PipelineType::Gfx:
		rootSig = this->m_gfxPipelinePool.Get(desc.GfxHandle)->RootSignature.Get();
		break;
	case PipelineType::Compute:
		rootSig = this->m_computePipelinePool.Get(desc.ComputeHandle)->RootSignature.Get();
		break;
	case PipelineType::Mesh:
		rootSig = this->m_meshPipelinePool.Get(desc.MeshHandle)->RootSignature.Get();
		break;
	default:
		rootSig = nullptr;
		break;
	}
	ThrowIfFailed(
		this->GetD3D12Device()->CreateCommandSignature(
			&cmdDesc,
			rootSig,
			IID_PPV_ARGS(&signature->NativeSignature)));

	return handle;
}

void phx::rhi::d3d12::D3D12GfxDevice::DeleteCommandSignature(CommandSignatureHandle handle)
{
	if (!handle.IsValid())
	{
		return;
	}

	DeleteItem d =
	{
		this->m_frameCount,
		[=]()
		{
			D3D12CommandSignature* signature = this->GetCommandSignaturePool().Get(handle);
			if (signature)
			{
				this->GetCommandSignaturePool().Release(handle);
			}
		}
	};

	this->m_deleteQueue.push_back(d);
}

ShaderHandle phx::rhi::d3d12::D3D12GfxDevice::CreateShader(ShaderDesc const& desc, Span<uint8_t> shaderByteCode)
{
	ShaderHandle handle = this->m_shaderPool.Emplace(desc, shaderByteCode.begin(), shaderByteCode.Size());
	D3D12Shader* shaderImpl = this->m_shaderPool.Get(handle);
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

	return handle;
}

InputLayoutHandle phx::rhi::d3d12::D3D12GfxDevice::CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount)
{
	InputLayoutHandle handle = this->m_inputLayoutPool.Emplace();
	D3D12InputLayout* inputLayoutImpl = this->m_inputLayoutPool.Get(handle);
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

	return handle;
}

GfxPipelineHandle phx::rhi::d3d12::D3D12GfxDevice::CreateGfxPipeline(GfxPipelineDesc const& desc)
{
	D3D12GraphicsPipeline pipeline = {};
	pipeline.Desc = desc;

	if (desc.VertexShader.IsValid())
	{
		D3D12Shader* shaderImpl = this->m_shaderPool.Get(desc.VertexShader);
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = shaderImpl->RootSignature;
		}
	}

	if (desc.PixelShader.IsValid())
	{
		D3D12Shader* shaderImpl = this->m_shaderPool.Get(desc.PixelShader);
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = shaderImpl->RootSignature;
		}
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12Desc = {};
	d3d12Desc.pRootSignature = pipeline.RootSignature.Get();

	D3D12Shader* shaderImpl = nullptr;
	shaderImpl = this->m_shaderPool.Get(desc.VertexShader);
	if (shaderImpl)
	{
		d3d12Desc.VS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = this->m_shaderPool.Get(desc.HullShader);
	if (shaderImpl)
	{
		d3d12Desc.HS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = this->m_shaderPool.Get(desc.DomainShader);
	if (shaderImpl)
	{
		d3d12Desc.DS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}


	shaderImpl = this->m_shaderPool.Get(desc.GeometryShader);
	if (shaderImpl)
	{
		d3d12Desc.GS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}
	;
	shaderImpl = this->m_shaderPool.Get(desc.PixelShader);
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

	D3D12InputLayout* inputLayout = this->m_inputLayoutPool.Get(desc.InputLayout);
	if (inputLayout && !inputLayout->InputElements.empty())
	{
		d3d12Desc.InputLayout.NumElements = uint32_t(inputLayout->InputElements.size());
		d3d12Desc.InputLayout.pInputElementDescs = &(inputLayout->InputElements[0]);
	}

	d3d12Desc.NumRenderTargets = (uint32_t)desc.RtvFormats.size();
	d3d12Desc.SampleMask = ~0u;

	ThrowIfFailed(
		this->GetD3D12Device2()->CreateGraphicsPipelineState(&d3d12Desc, IID_PPV_ARGS(&pipeline.D3D12PipelineState)));

	return this->m_gfxPipelinePool.Insert(pipeline);
}

void phx::rhi::d3d12::D3D12GfxDevice::DeleteGfxPipeline(GfxPipelineHandle handle)
{
	if (!handle.IsValid())
	{
		return;
	}

	DeleteItem d =
	{
		this->m_frameCount,
		[=]()
		{
			D3D12GraphicsPipeline* pipeline = this->GetGraphicsPipelinePool().Get(handle);
			if (pipeline)
			{
				this->GetGraphicsPipelinePool().Release(handle);
			}
		}
	};

	this->m_deleteQueue.push_back(d);
}

ComputePipelineHandle phx::rhi::d3d12::D3D12GfxDevice::CreateComputePipeline(ComputePipelineDesc const& desc)
{
	D3D12ComputePipeline pipeline = {};
	pipeline.Desc = desc;

	assert(desc.ComputeShader.IsValid());
	D3D12Shader* shaderImpl = this->m_shaderPool.Get(desc.ComputeShader);
	if (pipeline.RootSignature == nullptr)
	{
		pipeline.RootSignature = shaderImpl->RootSignature;
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12Desc = {};
	d3d12Desc.pRootSignature = pipeline.RootSignature.Get();
	d3d12Desc.CS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };

	ThrowIfFailed(
		this->GetD3D12Device2()->CreateComputePipelineState(&d3d12Desc, IID_PPV_ARGS(&pipeline.D3D12PipelineState)));

	return this->m_computePipelinePool.Insert(pipeline);
}

MeshPipelineHandle phx::rhi::d3d12::D3D12GfxDevice::CreateMeshPipeline(MeshPipelineDesc const& desc)
{
	D3D12MeshPipeline pipeline = {};
	pipeline.Desc = desc;

	if (desc.AmpShader.IsValid())
	{
		D3D12Shader* shaderImpl = this->m_shaderPool.Get(desc.AmpShader);
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = shaderImpl->RootSignature;
		}
	}

	if (desc.MeshShader.IsValid())
	{
		D3D12Shader* shaderImpl = this->m_shaderPool.Get(desc.MeshShader);
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = shaderImpl->RootSignature;
		}
	}

	if (desc.PixelShader.IsValid())
	{
		D3D12Shader* shaderImpl = this->m_shaderPool.Get(desc.PixelShader);
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = shaderImpl->RootSignature;
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

	psoDesc.RootSignature = pipeline.RootSignature.Get();

	D3D12Shader* shaderImpl = nullptr;
	shaderImpl = this->m_shaderPool.Get(desc.AmpShader);
	if (shaderImpl)
	{
		psoDesc.AmplificationShader = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = this->m_shaderPool.Get(desc.MeshShader);
	if (shaderImpl)
	{
		psoDesc.MeshShader = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = this->m_shaderPool.Get(desc.PixelShader);
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
		this->GetD3D12Device2()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipeline.D3D12PipelineState)));

	return this->m_meshPipelinePool.Insert(pipeline);
}

void D3D12GfxDevice::DeleteMeshPipeline(MeshPipelineHandle handle)
{
	if (!handle.IsValid())
	{
		return;
	}

	DeleteItem d =
	{
		this->m_frameCount,
		[=]()
		{
			D3D12MeshPipeline* pipeline = this->GetMeshPipelinePool().Get(handle);
			if (pipeline)
			{
				this->GetMeshPipelinePool().Release(handle);
			}
		}
	};

	this->m_deleteQueue.push_back(d);
}

TextureHandle phx::rhi::d3d12::D3D12GfxDevice::CreateTexture(TextureDesc const& desc)
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

	D3D12Texture textureImpl = {};
	textureImpl.Desc = desc;

	textureImpl.TotalSize = 0;
	textureImpl.Footprints.resize(desc.ArraySize * std::max(uint16_t(1u), desc.MipLevels));
	textureImpl.RowSizesInBytes.resize(textureImpl.Footprints.size());
	textureImpl.NumRows.resize(textureImpl.Footprints.size());
	this->GetD3D12Device2()->GetCopyableFootprints(
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
		this->m_d3d12MemAllocator->CreateResource(
			&allocationDesc,
			&resourceDesc,
			ConvertResourceStates(desc.InitialState),
			useClearValue ? &d3d12OptimizedClearValue : nullptr,
			&textureImpl.Allocation,
			IID_PPV_ARGS(&textureImpl.D3D12Resource)));


	std::wstring debugName;
	StringConvert(desc.DebugName, debugName);
	textureImpl.D3D12Resource->SetName(debugName.c_str());

	TextureHandle texture = this->m_texturePool.Insert(textureImpl);

	if ((desc.BindingFlags & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		this->CreateSubresource(texture, RHI::SubresouceType::SRV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
	{
		this->CreateSubresource(texture, RHI::SubresouceType::RTV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
	{
		this->CreateSubresource(texture, RHI::SubresouceType::DSV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		this->CreateSubresource(texture, RHI::SubresouceType::UAV, 0, ~0u, 0, ~0u);
	}

	return texture;
}

const TextureDesc& phx::rhi::d3d12::D3D12GfxDevice::GetTextureDesc(TextureHandle handle)
{
	const D3D12Texture* texture = this->m_texturePool.Get(handle);
	return texture->Desc;
}

DescriptorIndex phx::rhi::d3d12::D3D12GfxDevice::GetDescriptorIndex(TextureHandle handle, SubresouceType type, int subResource)
{
	const D3D12Texture* texture = this->m_texturePool.Get(handle);

	if (!texture)
	{
		return cInvalidDescriptorIndex;
	}

	switch (type)
	{
	case SubresouceType::SRV:
		return subResource == -1
			? texture->Srv.BindlessIndex
			: texture->SrvSubresourcesAlloc[subResource].BindlessIndex;
		break;

	case SubresouceType::UAV:
		return subResource == -1
			? texture->UavAllocation.BindlessIndex
			: texture->UavSubresourcesAlloc[subResource].BindlessIndex;
		break;

	case SubresouceType::RTV:
		return subResource == -1
			? texture->RtvAllocation.BindlessIndex
			: texture->RtvSubresourcesAlloc[subResource].BindlessIndex;
		break;

	case SubresouceType::DSV:
		return subResource == -1
			? texture->DsvAllocation.BindlessIndex
			: texture->DsvSubresourcesAlloc[subResource].BindlessIndex;
		break;
	default:
		throw std::runtime_error("Unsupported enum type");
	}
}

void phx::rhi::d3d12::D3D12GfxDevice::DeleteTexture(TextureHandle handle)
{
	if (!handle.IsValid())
	{
		return;
	}

	DeleteItem d =
	{
		this->m_frameCount,
		[=]()
		{
			D3D12Texture* texture = this->m_texturePool.Get(handle);

			if (texture)
			{
				for (auto& view : texture->SrvSubresourcesAlloc)
				{
					if (view.BindlessIndex != cInvalidDescriptorIndex)
					{
						this->m_bindlessResourceDescriptorTable.Free(view.BindlessIndex);
					}
				}

				for (auto& view : texture->UavSubresourcesAlloc)
				{
					if (view.BindlessIndex != cInvalidDescriptorIndex)
					{
						this->m_bindlessResourceDescriptorTable.Free(view.BindlessIndex);
					}
				}

				texture->DisposeViews();
				this->GetTexturePool().Release(handle);
			}
		}
	};

	this->m_deleteQueue.push_back(d);
}

BufferHandle phx::rhi::d3d12::D3D12GfxDevice::CreateBuffer(BufferDesc const& desc)
{
	BufferHandle buffer = this->m_bufferPool.Emplace();
	D3D12Buffer& bufferImpl = *this->m_bufferPool.Get(buffer);
	this->CreateBufferInternal(desc, bufferImpl);

	if ((desc.MiscFlags & BufferMiscFlags::IsAliasedResource) == BufferMiscFlags::IsAliasedResource)
	{
		return buffer;
	}

	if ((desc.Binding & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		this->CreateSubresource(buffer, RHI::SubresouceType::SRV, 0u);
	}

	if ((desc.Binding & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		this->CreateSubresource(buffer, RHI::SubresouceType::UAV, 0u);
	}

	return buffer;
}

const BufferDesc& D3D12GfxDevice::GetBufferDesc(BufferHandle handle)
{
	return this->m_bufferPool.Get(handle)->Desc;
}

DescriptorIndex D3D12GfxDevice::GetDescriptorIndex(BufferHandle handle, SubresouceType type, int subResource)
{
	const D3D12Buffer* bufferImpl = this->m_bufferPool.Get(handle);

	if (!bufferImpl)
	{
		return cInvalidDescriptorIndex;
	}

	switch (type)
	{
	case SubresouceType::SRV:
		return subResource == -1
			? bufferImpl->Srv.BindlessIndex
			: bufferImpl->SrvSubresourcesAlloc[subResource].BindlessIndex;

	case SubresouceType::UAV:
		return subResource == -1
			? bufferImpl->UavAllocation.BindlessIndex
			: bufferImpl->UavSubresourcesAlloc[subResource].BindlessIndex;
	default:
		throw std::runtime_error("Unsupported enum type");
	}
}

void* D3D12GfxDevice::GetBufferMappedData(BufferHandle handle)
{
	const D3D12Buffer* bufferImpl = this->m_bufferPool.Get(handle);
	assert(bufferImpl->GetDesc().Usage != Usage::Default);
	return bufferImpl->MappedData;
}

uint32_t D3D12GfxDevice::GetBufferMappedDataSizeInBytes(BufferHandle handle)
{
	const D3D12Buffer* bufferImpl = this->m_bufferPool.Get(handle);
	assert(bufferImpl->GetDesc().Usage != Usage::Default);
	return bufferImpl->MappedSizeInBytes;
}

void D3D12GfxDevice::DeleteBuffer(BufferHandle handle)
{
	if (!handle.IsValid())
	{
		return;
	}
	D3D12Buffer* bufferImpl = this->m_bufferPool.Get(handle);

	DeleteItem d =
	{
		this->m_frameCount,
		[=]()
		{
			D3D12Buffer* bufferImpl = this->m_bufferPool.Get(handle);

			if (bufferImpl)
			{
				for (auto& view : bufferImpl->SrvSubresourcesAlloc)
				{
					if (view.BindlessIndex != cInvalidDescriptorIndex)
					{
						this->m_bindlessResourceDescriptorTable.Free(view.BindlessIndex);
					}
				}

				for (auto& view : bufferImpl->UavSubresourcesAlloc)
				{
					if (view.BindlessIndex != cInvalidDescriptorIndex)
					{
						this->m_bindlessResourceDescriptorTable.Free(view.BindlessIndex);
					}
				}

				bufferImpl->DisposeViews();
				this->GetBufferPool().Release(handle);
			}
		}
	};

	this->m_deleteQueue.push_back(d);
}

RTAccelerationStructureHandle phx::rhi::d3d12::D3D12GfxDevice::CreateRTAccelerationStructure(RTAccelerationStructureDesc const& desc)
{
	RTAccelerationStructureHandle handle = this->m_rtAccelerationStructurePool.Emplace();
	D3D12RTAccelerationStructure& rtAccelerationStructureImpl = *this->m_rtAccelerationStructurePool.Get(handle);
	rtAccelerationStructureImpl.Desc = desc;

	if (desc.Flags & RTAccelerationStructureDesc::FLAGS::kAllowUpdate)
	{
		rtAccelerationStructureImpl.Dx12Desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}

	if (desc.Flags & RTAccelerationStructureDesc::FLAGS::kAllowCompaction)
	{
		rtAccelerationStructureImpl.Dx12Desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
	}

	if (desc.Flags & RTAccelerationStructureDesc::FLAGS::kPreferFastTrace)
	{
		rtAccelerationStructureImpl.Dx12Desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	}

	if (desc.Flags & RTAccelerationStructureDesc::FLAGS::kPreferFastBuild)
	{
		rtAccelerationStructureImpl.Dx12Desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	}

	if (desc.Flags & RTAccelerationStructureDesc::FLAGS::kMinimizeMemory)
	{
		rtAccelerationStructureImpl.Dx12Desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
	}


	switch (desc.Type)
	{
	case RTAccelerationStructureDesc::Type::BottomLevel:
	{
		rtAccelerationStructureImpl.Dx12Desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		rtAccelerationStructureImpl.Dx12Desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

		for (auto& g : desc.ButtomLevel.Geometries)
		{
			D3D12_RAYTRACING_GEOMETRY_DESC& dx12GeometryDesc = rtAccelerationStructureImpl.Geometries.emplace_back();
			dx12GeometryDesc = {};

			if (g.Type == RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::Triangles)
			{
				dx12GeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

				D3D12Buffer* dx12VertexBuffer = this->m_bufferPool.Get(g.Triangles.VertexBuffer);
				assert(dx12VertexBuffer);
				D3D12Buffer* dx12IndexBuffer = this->m_bufferPool.Get(g.Triangles.IndexBuffer);
				assert(dx12IndexBuffer);

				D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& trianglesDesc = dx12GeometryDesc.Triangles;
				trianglesDesc.VertexFormat = ConvertFormat(g.Triangles.VertexFormat);
				trianglesDesc.VertexCount = (UINT)g.Triangles.VertexCount;
				trianglesDesc.VertexBuffer.StartAddress = dx12VertexBuffer->VertexView.BufferLocation + (D3D12_GPU_VIRTUAL_ADDRESS)g.Triangles.VertexByteOffset;
				trianglesDesc.VertexBuffer.StrideInBytes = g.Triangles.VertexStride;
				trianglesDesc.IndexBuffer =
					dx12VertexBuffer->IndexView.BufferLocation +
					(D3D12_GPU_VIRTUAL_ADDRESS)g.Triangles.IndexOffset * (g.Triangles.IndexFormat == RHI::Format::R16_UINT ? sizeof(uint16_t) : sizeof(uint32_t));
				trianglesDesc.IndexCount = g.Triangles.IndexCount;
				trianglesDesc.IndexFormat = g.Triangles.IndexFormat == RHI::Format::R16_UINT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

				if (g.Flags & RTAccelerationStructureDesc::BottomLevelDesc::Geometry::kUseTransform)
				{
					D3D12Buffer* transform3x4Buffer = this->m_bufferPool.Get(g.Triangles.Transform3x4Buffer);
					assert(transform3x4Buffer);
					trianglesDesc.Transform3x4 = transform3x4Buffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)g.Triangles.Transform3x4BufferOffset;
				}
			}
			else if (g.Type == RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::ProceduralAABB)
			{
				dx12GeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;

				D3D12Buffer* aabbBuffer = this->m_bufferPool.Get(g.AABBs.AABBBuffer);
				assert(aabbBuffer);
				dx12GeometryDesc.AABBs.AABBs.StartAddress = aabbBuffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)g.AABBs.Offset;
				dx12GeometryDesc.AABBs.AABBs.StrideInBytes = (UINT64)g.AABBs.Stride;
				dx12GeometryDesc.AABBs.AABBCount = g.AABBs.Stride;
			}
		}
		rtAccelerationStructureImpl.Dx12Desc.pGeometryDescs = rtAccelerationStructureImpl.Geometries.data();
		rtAccelerationStructureImpl.Dx12Desc.NumDescs = (UINT)rtAccelerationStructureImpl.Geometries.size();
		break;
	}
	case RTAccelerationStructureDesc::Type::TopLevel:
	{
		rtAccelerationStructureImpl.Dx12Desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		rtAccelerationStructureImpl.Dx12Desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

		D3D12Buffer* instanceBuffer = this->m_bufferPool.Get(desc.TopLevel.InstanceBuffer);
		assert(instanceBuffer);
		rtAccelerationStructureImpl.Dx12Desc.InstanceDescs = instanceBuffer->D3D12Resource->GetGPUVirtualAddress() +
			(D3D12_GPU_VIRTUAL_ADDRESS)desc.TopLevel.Offset;
		rtAccelerationStructureImpl.Dx12Desc.NumDescs = (UINT)desc.TopLevel.Count;

		break;
	}
	default:
		return {};
	}

	this->GetD3D12Device5()->GetRaytracingAccelerationStructurePrebuildInfo(
		&rtAccelerationStructureImpl.Dx12Desc,
		&rtAccelerationStructureImpl.Info);


	UINT64 alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
	UINT64 alignedSize = MemoryAlign(rtAccelerationStructureImpl.Info.ResultDataMaxSizeInBytes, alignment);

	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.Width = (UINT64)alignedSize;
	resourceDesc.Height = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Alignment = 0;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;

	D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;


	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
	ThrowIfFailed(
		this->m_d3d12MemAllocator->CreateResource(
			&allocationDesc,
			&resourceDesc,
			resourceState,
			nullptr,
			&rtAccelerationStructureImpl.Allocation,
			IID_PPV_ARGS(&rtAccelerationStructureImpl.D3D12Resource)));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.RaytracingAccelerationStructure.Location = rtAccelerationStructureImpl.D3D12Resource->GetGPUVirtualAddress();


	rtAccelerationStructureImpl.Srv = {
			.Allocation = this->GetResourceCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.SRVDesc = srvDesc,
	};

	this->GetD3D12Device5()->CreateShaderResourceView(
		nullptr,
		&srvDesc,
		rtAccelerationStructureImpl.Srv.Allocation.GetCpuHandle());

	// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
	rtAccelerationStructureImpl.Srv.BindlessIndex = this->m_bindlessResourceDescriptorTable.Allocate();
	if (rtAccelerationStructureImpl.Srv.BindlessIndex != cInvalidDescriptorIndex)
	{
		this->GetD3D12Device5()->CopyDescriptorsSimple(
			1,
			this->m_bindlessResourceDescriptorTable.GetCpuHandle(rtAccelerationStructureImpl.Srv.BindlessIndex),
			rtAccelerationStructureImpl.Srv.Allocation.GetCpuHandle(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	BufferDesc scratchBufferDesc = {};
	rtAccelerationStructureImpl.SratchBuffer;
	scratchBufferDesc.Stride = (uint32_t)std::max(rtAccelerationStructureImpl.Info.ScratchDataSizeInBytes, rtAccelerationStructureImpl.Info.UpdateScratchDataSizeInBytes);
	scratchBufferDesc.NumElements = 1;
	scratchBufferDesc.DebugName = "RT Scratch Buffer";
	rtAccelerationStructureImpl.SratchBuffer = this->CreateBuffer(scratchBufferDesc);

	return handle;
}

const RTAccelerationStructureDesc& phx::rhi::d3d12::D3D12GfxDevice::GetRTAccelerationStructureDesc(RTAccelerationStructureHandle handle)
{
	assert(handle.IsValid());
	return this->m_rtAccelerationStructurePool.Get(handle)->Desc;
}

void phx::rhi::d3d12::D3D12GfxDevice::WriteRTTopLevelAccelerationStructureInstance(RTAccelerationStructureDesc::TopLevelDesc::Instance const& instance, void* dest)
{
	assert(instance.BottomLevel.IsValid());

	D3D12_RAYTRACING_INSTANCE_DESC tmp;
	tmp.AccelerationStructure = this->m_rtAccelerationStructurePool.Get(instance.BottomLevel)->D3D12Resource->GetGPUVirtualAddress();
	std::memcpy(tmp.Transform, &instance.Transform, sizeof(tmp.Transform));
	tmp.InstanceID = instance.InstanceId;
	tmp.InstanceMask = instance.InstanceMask;
	tmp.InstanceContributionToHitGroupIndex = instance.InstanceContributionToHitGroupIndex;
	tmp.Flags = instance.Flags;
	tmp.Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
	std::memcpy(dest, &tmp, sizeof(D3D12_RAYTRACING_INSTANCE_DESC)); // memcpy whole structure into mapped pointer to avoid read from uncached memory
}

size_t phx::rhi::d3d12::D3D12GfxDevice::GetRTTopLevelAccelerationStructureInstanceSize()
{
	return sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
}

void phx::rhi::d3d12::D3D12GfxDevice::DeleteRtAccelerationStructure(RTAccelerationStructureHandle handle)
{
	if (!handle.IsValid())
	{
		return;
	}

	D3D12RTAccelerationStructure* impl = this->m_rtAccelerationStructurePool.Get(handle);
	this->DeleteBuffer(impl->SratchBuffer);

	if (impl->Desc.Type == RTAccelerationStructureDesc::Type::TopLevel)
	{
		this->DeleteBuffer(impl->Desc.TopLevel.InstanceBuffer);
	}

	DeleteItem d =
	{
		this->m_frameCount,
		[=]()
		{
			if (impl)
			{
				if (impl->Srv.BindlessIndex != cInvalidDescriptorIndex)
				{
					this->m_bindlessResourceDescriptorTable.Free(impl->Srv.BindlessIndex);
				}

				this->m_rtAccelerationStructurePool.Release(handle);
			}
		}
	};

	this->m_deleteQueue.push_back(d);
}

DescriptorIndex phx::rhi::d3d12::D3D12GfxDevice::GetDescriptorIndex(RTAccelerationStructureHandle handle)
{
	const D3D12RTAccelerationStructure* impl = this->m_rtAccelerationStructurePool.Get(handle);

	if (!impl)
	{
		return cInvalidDescriptorIndex;
	}

	return impl->Srv.BindlessIndex;
}

int phx::rhi::d3d12::D3D12GfxDevice::CreateSubresource(TextureHandle texture, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
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

int phx::rhi::d3d12::D3D12GfxDevice::CreateSubresource(BufferHandle buffer, SubresouceType subresourceType, size_t offset, size_t size)
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

TimerQueryHandle phx::rhi::d3d12::D3D12GfxDevice::CreateTimerQuery()
{

	int queryIndex = this->m_timerQueryIndexPool.Allocate();
	if (queryIndex < 0)
	{
		assert(false);
		return {};
	}

	TimerQueryHandle handle = this->m_timerQueryPool.Emplace();

	D3D12TimerQuery& timerQuery = *this->m_timerQueryPool.Get(handle);

	timerQuery.BeginQueryIndex = queryIndex * 2;
	timerQuery.EndQueryIndex = timerQuery.BeginQueryIndex + 1;
	timerQuery.Resolved = false;
	timerQuery.Time = {};


	return handle;
}

void phx::rhi::d3d12::D3D12GfxDevice::DeleteTimerQuery(TimerQueryHandle query)
{
	if (!query.IsValid())
	{
		return;
	}

	DeleteItem d =
	{
		this->m_frameCount,
		[=]()
		{
			D3D12TimerQuery* impl = this->m_timerQueryPool.Get(query);

			if (impl)
			{
				this->m_timerQueryIndexPool.Release(static_cast<int>(impl->BeginQueryIndex) / 2);
				this->m_timerQueryPool.Release(query);
			}
		}
	};

	this->m_deleteQueue.push_back(d);
}

bool phx::rhi::d3d12::D3D12GfxDevice::PollTimerQuery(TimerQueryHandle query)
{
	assert(query.IsValid());
	D3D12TimerQuery* queryImpl = this->m_timerQueryPool.Get(query);

	if (!queryImpl->Started)
	{
		return false;
	}

	if (!queryImpl->CommandQueue)
	{
		return true;
	}

	if (queryImpl->CommandQueue->GetLastCompletedFence() >= queryImpl->FenceCount)
	{
		queryImpl->CommandQueue = nullptr;
		return true;
	}

	return false;
}

TimeStep phx::rhi::d3d12::D3D12GfxDevice::GetTimerQueryTime(TimerQueryHandle query)
{
	assert(query.IsValid());
	D3D12TimerQuery* queryImpl = this->m_timerQueryPool.Get(query);

	if (queryImpl->Resolved)
	{
		return queryImpl->Time;
	}

	if (queryImpl->CommandQueue)
	{
		queryImpl->CommandQueue->WaitForFence(queryImpl->FenceCount);
		queryImpl->CommandQueue = nullptr;
	}

	// Could use the queue that it was executued on
	// The GPU timestamp counter frequency (in ticks/second)
	uint64_t frequency;
	this->GetGfxQueue().GetD3D12CommandQueue()->GetTimestampFrequency(&frequency);

	D3D12_RANGE bufferReadRange = {
	queryImpl->BeginQueryIndex * sizeof(uint64_t),
	(queryImpl->BeginQueryIndex + 2) * sizeof(uint64_t) };

	const D3D12Buffer* timestampQueryBuffer = this->m_bufferPool.Get(this->m_timestampQueryBuffer);
	uint64_t* Data;
	const HRESULT res = timestampQueryBuffer->D3D12Resource->Map(0, &bufferReadRange, (void**)&Data);

	if (FAILED(res))
	{
		return TimeStep(0.0f);
	}

	queryImpl->Resolved = true;
	queryImpl->Time = TimeStep(float(double(Data[queryImpl->EndQueryIndex] - Data[queryImpl->BeginQueryIndex]) / double(frequency)));

	timestampQueryBuffer->D3D12Resource->Unmap(0, nullptr);

	return queryImpl->Time;
}

void phx::rhi::d3d12::D3D12GfxDevice::ResetTimerQuery(TimerQueryHandle query)
{
	assert(query.IsValid());
	D3D12TimerQuery* queryImpl = this->m_timerQueryPool.Get(query);

	queryImpl->Started = false;
	queryImpl->Resolved = false;
	queryImpl->Time = 0.f;
	queryImpl->CommandQueue = nullptr;
}

inline TextureHandle  phx::rhi::d3d12::D3D12GfxDevice::D3D12GfxDevice::GetBackBuffer()
{
	return this->m_swapChain.BackBuffers[this->GetBackBufferIndex()];
}

size_t phx::rhi::d3d12::D3D12GfxDevice::GetBackBufferIndex() const
{
	auto retVal = this->m_swapChain.NativeSwapchain4->GetCurrentBackBufferIndex();
	return (size_t)retVal;
}

void phx::rhi::d3d12::D3D12GfxDevice::BeginCapture(std::wstring const& filename)
{
#if ENABLE_PIX_CAPUTRE
	if (this->m_pixCaptureModule)
	{
		PIXCaptureParameters param = {};
		param.GpuCaptureParameters.FileName = filename.c_str();
		PIXBeginCapture(PIX_CAPTURE_GPU, &param);
	}
#endif
}

void phx::rhi::d3d12::D3D12GfxDevice::EndCapture(bool discard)
{
#if ENABLE_PIX_CAPUTRE
	if (this->m_pixCaptureModule)
	{
		PIXEndCapture(discard);
	}
#endif
}

bool phx::rhi::d3d12::D3D12GfxDevice::CheckCapability(DeviceCapability deviceCapability)
{
	return (this->m_capabilities & deviceCapability) == deviceCapability;
}

float phx::rhi::d3d12::D3D12GfxDevice::GetAvgFrameTime()
{
	return 0.0f;
}

bool phx::rhi::d3d12::D3D12GfxDevice::IsDevicedRemoved()
{
	HRESULT hr = this->GetD3D12Device5()->GetDeviceRemovedReason();
	return FAILED(hr);
}

void phx::rhi::d3d12::D3D12GfxDevice::DeleteD3DResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
{
	DeleteItem d =
	{
		this->m_frameCount,
		[=]()
		{
			resource;
		}
	};

	this->m_deleteQueue.push_back(d);
}

TextureHandle phx::rhi::d3d12::D3D12GfxDevice::CreateRenderTarget(
	TextureDesc const& desc,
	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12TextureResource)
{
	// Construct Texture
	D3D12Texture texture = {};
	texture.Desc = desc;
	texture.D3D12Resource = d3d12TextureResource;
	texture.RtvAllocation =
	{
		.Allocation = this->GetRtvCpuHeap().Allocate(1),
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV
	};

	this->GetD3D12Device2()->CreateRenderTargetView(
		texture.D3D12Resource.Get(),
		nullptr,
		texture.RtvAllocation.Allocation.GetCpuHandle());

	// Set debug name
	std::wstring debugName(texture.Desc.DebugName.begin(), texture.Desc.DebugName.end());
	texture.D3D12Resource->SetName(debugName.c_str());

	return this->m_texturePool.Insert(texture);
}

TextureHandle phx::rhi::d3d12::D3D12GfxDevice::CreateTexture(TextureDesc const& desc, Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource)
{
	// Construct Texture
	D3D12Texture texture = {};
	texture.Desc = desc;
	texture.D3D12Resource = d3d12Resource;
	texture.RtvAllocation =
	{
		.Allocation = this->GetRtvCpuHeap().Allocate(1),
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV
	};

	this->GetD3D12Device2()->CreateRenderTargetView(
		texture.D3D12Resource.Get(),
		nullptr,
		texture.RtvAllocation.Allocation.GetCpuHandle());

	// Set debug name
	std::wstring debugName(texture.Desc.DebugName.begin(), texture.Desc.DebugName.end());
	texture.D3D12Resource->SetName(debugName.c_str());

	TextureHandle textureHande = this->m_texturePool.Insert(texture);

	if ((desc.BindingFlags & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		this->CreateSubresource(textureHande, RHI::SubresouceType::SRV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
	{
		this->CreateSubresource(textureHande, RHI::SubresouceType::RTV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
	{
		this->CreateSubresource(textureHande, RHI::SubresouceType::DSV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		this->CreateSubresource(textureHande, RHI::SubresouceType::UAV, 0, ~0u, 0, ~0u);
	}

	return textureHande;
}

int phx::rhi::d3d12::D3D12GfxDevice::CreateShaderResourceView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	// TODO: Make use of parameters.
	D3D12Texture* textureImpl = this->m_texturePool.Get(texture);

	auto dxgiFormatMapping = GetDxgiFormatMapping(textureImpl->Desc.Format);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = dxgiFormatMapping.SrvFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	uint32_t planeSlice = (srvDesc.Format == DXGI_FORMAT_X24_TYPELESS_G8_UINT) ? 1 : 0;

	if (textureImpl->Desc.Dimension == TextureDimension::TextureCube)
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
	switch (textureImpl->Desc.Dimension)
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
		srvDesc.TextureCubeArray.NumCubes = textureImpl->Desc.ArraySize / 6;// Subresource data
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

	if ((textureImpl->Desc.MiscFlags | TextureMiscFlags::Bindless) == TextureMiscFlags::Bindless)
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

int phx::rhi::d3d12::D3D12GfxDevice::CreateRenderTargetView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	D3D12Texture* textureImpl = this->m_texturePool.Get(texture);

	auto dxgiFormatMapping = GetDxgiFormatMapping(textureImpl->Desc.Format);
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = dxgiFormatMapping.RtvFormat;

	switch (textureImpl->Desc.Dimension)
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
		rtvDesc.Texture3D.WSize = textureImpl->Desc.ArraySize;
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

int phx::rhi::d3d12::D3D12GfxDevice::CreateDepthStencilView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	D3D12Texture* textureImpl = this->m_texturePool.Get(texture);

	auto dxgiFormatMapping = GetDxgiFormatMapping(textureImpl->Desc.Format);
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = dxgiFormatMapping.RtvFormat;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	switch (textureImpl->Desc.Dimension)
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

int phx::rhi::d3d12::D3D12GfxDevice::CreateUnorderedAccessView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	D3D12Texture* textureImpl = this->m_texturePool.Get(texture);

	auto dxgiFormatMapping = GetDxgiFormatMapping(textureImpl->Desc.Format);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = dxgiFormatMapping.SrvFormat;

	switch (textureImpl->Desc.Dimension)
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
		uavDesc.Texture3D.WSize = textureImpl->Desc.Depth;
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

	if ((textureImpl->Desc.MiscFlags | TextureMiscFlags::Bindless) == TextureMiscFlags::Bindless)
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

/*
RootSignatureHandle phx::rhi::Dx12::GraphicsDevice::CreateRootSignature(GraphicsPipelineDesc const& desc)
{
	/*
	auto rootSignatureImpl = std::make_unique<RootSignature>();

	assert(desc.RootSignatureBuilder);
	RootSignatureBuilder* rootSignatureBuilder = SafeCast<RootSignatureBuilder*>(desc.RootSignatureBuilder);

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

	rsDesc.Desc_1_1 = rootSignatureBuilder->Build();

	if (desc.InputLayout)
	{
		rsDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

	std::shared_ptr<ID3DBlob> rsBlob;
	std::shared_ptr<ID3DBlob> errorBlob;
	ThrowIfFailed(
		D3D12SerializeVersionedRootSignature(&rsDesc, &rsBlob, &errorBlob));

	ThrowIfFailed(
		this->GetD3D12Device2()->CreateRootSignature(
			0,
			rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatureImpl->D3D12RootSignature)));

	return RootSignatureHandle::Create(rootSignatureImpl.release());
	std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;

	for (const auto& parameter : desc.ShaderParameters.Parameters)
	{
		switch (parameter.Type)
		{
		case ResourceType::PushConstants:
			rootSignatureImpl->PushConstantByteSize = parameter.Size / 4;

			rootParameters.emplace_back().InitAsConstants(
				rootSignatureImpl->PushConstantByteSize,
				parameter.Slot);
			break;

		case ResourceType::ConstantBuffer:
			rootParameters.emplace_back().InitAsConstantBufferView(
				parameter.Slot,
				0,
				parameter.IsVolatile ? D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC : D3D12_ROOT_DESCRIPTOR_FLAG_NONE);
			break;

		case ResourceType::SRVBuffer:
			rootParameters.emplace_back().InitAsShaderResourceView(
				parameter.Slot,
				0,
				parameter.IsVolatile ? D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC : D3D12_ROOT_DESCRIPTOR_FLAG_NONE);
			break;

		case ResourceType::SRVTexture:
			assert(false);
			break;
		}
	}

	// Create Enable Bindless Table
	if (!desc.ShaderParameters.BindlessParameters.empty())
	{
		std::vector<CD3DX12_DESCRIPTOR_RANGE1> bindlessRanges;
		rootSignatureImpl->BindBindlessTables = true;
		for (const auto& bindlessParam : desc.ShaderParameters.BindlessParameters)
		{
			auto& range = bindlessRanges.emplace_back();
			switch (bindlessParam.Type)
			{
			case ResourceType::BindlessSRV:
				range.GetRange()Type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				break;
			default:
				throw std::exception("Not implemented yet");
			}
			range.BaseShaderRegister = 0;
			range.RegisterSpace = bindlessParam.RegisterSpace;
			range.OffsetInDescriptorsFromTableStart = 0;
			range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
		}

		rootParameters.emplace_back().InitAsDescriptorTable(
			bindlessRanges.size(),
			bindlessRanges.data());
	}

	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers(desc.ShaderParameters.StaticSamplers.size());
	for (int i = 0; i < staticSamplers.size(); i++)
	{
		const auto& sampler = desc.ShaderParameters.StaticSamplers[i];

		staticSamplers[i].ShaderRegister = sampler.Slot;
		staticSamplers[i].RegisterSpace = 0;

		UINT reductionType = ConvertSamplerReductionType(sampler.ReductionType);
		if (sampler.MaxAnisotropy > 1.0f)
		{
			staticSamplers[i].Filter = D3D12_ENCODE_ANISOTROPIC_FILTER(reductionType);
		}
		else
		{
			staticSamplers[i].Filter = D3D12_ENCODE_BASIC_FILTER(
				sampler.MinFilter ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
				sampler.MagFilter ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
				sampler.MipFilter ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
				reductionType);
		}

		staticSamplers[i].AddressU = ConvertSamplerAddressMode(sampler.AddressU);
		staticSamplers[i].AddressV = ConvertSamplerAddressMode(sampler.AddressV);
		staticSamplers[i].AddressW = ConvertSamplerAddressMode(sampler.AddressW);

		staticSamplers[i].MipLODBias = sampler.MipBias;
		staticSamplers[i].MaxAnisotropy = std::max((UINT)sampler.MaxAnisotropy, 1U);
		staticSamplers[i].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
		staticSamplers[i].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		staticSamplers[i].MinLOD = 0;
		staticSamplers[i].MaxLOD = D3D12_FLOAT32_MAX;
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (desc.InputLayout)
	{
		rsDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

	if (!rootParameters.empty())
	{
		rsDesc.Desc_1_1.pParameters = rootParameters.data();
		rsDesc.Desc_1_1.NumParameters = UINT(rootParameters.size());
	}

	if (!staticSamplers.empty())
	{
		rsDesc.Desc_1_1.pStaticSamplers = staticSamplers.data();
		rsDesc.Desc_1_1.NumStaticSamplers = UINT(staticSamplers.size());
	}
	// Serialize the root signature

	std::shared_ptr<ID3DBlob> rsBlob;
	std::shared_ptr<ID3DBlob> errorBlob;
	ThrowIfFailed(
		D3D12SerializeVersionedRootSignature(&rsDesc, &rsBlob, &errorBlob));

	ThrowIfFailed(
		this->GetD3D12Device2()->CreateRootSignature(
			0,
			rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatureImpl->D3D12RootSignature)));

	return RootSignatureHandle::Create(rootSignatureImpl.release());
}
	*/


void phx::rhi::d3d12::D3D12GfxDevice::RunGarbageCollection(uint64_t completedFrame)
{
	PHX_EVENT();
	// OPTICK_TAG("Delete Queue Size", this->m_deleteQueue.size());
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

void phx::rhi::d3d12::D3D12GfxDevice::TranslateBlendState(BlendRenderState const& inState, D3D12_BLEND_DESC& outState)
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

void phx::rhi::d3d12::D3D12GfxDevice::TranslateDepthStencilState(DepthStencilRenderState const& inState, D3D12_DEPTH_STENCIL_DESC& outState)
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

void phx::rhi::d3d12::D3D12GfxDevice::TranslateRasterState(RasterRenderState const& inState, D3D12_RASTERIZER_DESC& outState)
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

void D3D12GfxDevice::CreateBufferInternal(BufferDesc const& desc, D3D12Buffer& outBuffer)
{
	outBuffer.Desc = desc;

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
		alignedSize = MemoryAlign(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.Stride * desc.NumElements, resourceFlags, alignedSize);

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

	if ((outBuffer.Desc.MiscFlags & BufferMiscFlags::IsAliasedResource) == BufferMiscFlags::IsAliasedResource)
	{
		D3D12_RESOURCE_ALLOCATION_INFO finalAllocInfo = {};
		finalAllocInfo.Alignment = 0;
		finalAllocInfo.SizeInBytes = MemoryAlign(
			outBuffer.Desc.Stride * outBuffer.Desc.NumElements,
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * 1024);

		ThrowIfFailed(
			this->m_d3d12MemAllocator->AllocateMemory(
				&allocationDesc,
				&finalAllocInfo,
				&outBuffer.Allocation));
		return;
	}
	else if (outBuffer.Desc.AliasedBuffer.IsValid())
	{
		D3D12Buffer* aliasedBuffer = this->m_bufferPool.Get(outBuffer.Desc.AliasedBuffer);

		ThrowIfFailed(
			this->m_d3d12MemAllocator->CreateAliasingResource(
				aliasedBuffer->Allocation.Get(),
				0,
				&resourceDesc,
				initialState,
				nullptr,
				IID_PPV_ARGS(&outBuffer.D3D12Resource)));
	}
	else
	{
		ThrowIfFailed(
			this->m_d3d12MemAllocator->CreateResource(
				&allocationDesc,
				&resourceDesc,
				initialState,
				nullptr,
				&outBuffer.Allocation,
				IID_PPV_ARGS(&outBuffer.D3D12Resource)));
	}

	switch (desc.Usage)
	{
	case Usage::ReadBack:
	{
		ThrowIfFailed(
			outBuffer.D3D12Resource->Map(0, nullptr, &outBuffer.MappedData));
		outBuffer.MappedSizeInBytes = static_cast<uint32_t>(desc.Stride * desc.NumElements);

		break;
	}

	case Usage::Upload:
	{
		D3D12_RANGE readRange = {};
		ThrowIfFailed(
			outBuffer.D3D12Resource->Map(0, &readRange, &outBuffer.MappedData));

		outBuffer.MappedSizeInBytes = static_cast<uint32_t>(desc.Stride * desc.NumElements);
		break;
	}
	case Usage::Default:
	default:
		break;
	}

	std::wstring debugName(desc.DebugName.begin(), desc.DebugName.end());
	outBuffer.D3D12Resource->SetName(debugName.c_str());
}

int phx::rhi::d3d12::D3D12GfxDevice::CreateShaderResourceView(BufferHandle buffer, size_t offset, size_t size)
{
	D3D12Buffer* bufferImpl = this->m_bufferPool.Get(buffer);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	const uint32_t stideInBytes = bufferImpl->Desc.Stride * bufferImpl->Desc.NumElements;
	if (bufferImpl->Desc.Format == RHI::Format::UNKNOWN)
	{
		if (EnumHasAllFlags(bufferImpl->Desc.MiscFlags, BufferMiscFlags::Raw))
		{
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			srvDesc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
			srvDesc.Buffer.NumElements = (UINT)std::min(size, stideInBytes - offset) / sizeof(uint32_t);
		}
		else if (EnumHasAllFlags(bufferImpl->Desc.MiscFlags, BufferMiscFlags::Structured))
		{
			uint32_t strideInBytes = (bufferImpl->Desc.Stride * bufferImpl->Desc.NumElements);
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

int phx::rhi::d3d12::D3D12GfxDevice::CreateUnorderedAccessView(BufferHandle buffer, size_t offset, size_t size)
{
	D3D12Buffer* bufferImpl = this->m_bufferPool.Get(buffer);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

	const uint32_t stideInBytes = bufferImpl->Desc.Stride * bufferImpl->Desc.NumElements;
	const bool hasCounter = (bufferImpl->Desc.MiscFlags & BufferMiscFlags::HasCounter) == BufferMiscFlags::HasCounter;
	if (bufferImpl->Desc.Format == RHI::Format::UNKNOWN)
	{
		if (EnumHasAllFlags(bufferImpl->Desc.MiscFlags, BufferMiscFlags::Raw))
		{
			uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
			uavDesc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
			uavDesc.Buffer.NumElements = (UINT)std::min(size, stideInBytes - offset) / sizeof(uint32_t);
		}
		else if (EnumHasAllFlags(bufferImpl->Desc.MiscFlags, BufferMiscFlags::Structured))
		{
			uint32_t strideInBytes = (bufferImpl->Desc.Stride * bufferImpl->Desc.NumElements);
			// This is a Structured Buffer
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.Buffer.FirstElement = (UINT)offset / strideInBytes;
			uavDesc.Buffer.NumElements = (UINT)std::min(size, stideInBytes - offset) / strideInBytes;
			uavDesc.Buffer.StructureByteStride = strideInBytes;

			if (hasCounter)
			{
				uavDesc.Buffer.CounterOffsetInBytes = bufferImpl->Desc.UavCounterOffset;
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
		if (bufferImpl->Desc.UavCounterBuffer.IsValid())
		{
			D3D12Buffer* counterBuffer = this->m_bufferPool.Get(bufferImpl->Desc.UavCounterBuffer);
			counterResource = counterBuffer->D3D12Resource.Get();

		}
		else
		{
			counterResource = bufferImpl->D3D12Resource.Get();
		}
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

void D3D12GfxDevice::CreateGpuTimestampQueryHeap(uint32_t queryCount)
{
	D3D12_QUERY_HEAP_DESC dx12Desc = {};
	dx12Desc.Count = queryCount;
	dx12Desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

	ThrowIfFailed(
		this->m_rootDevice->CreateQueryHeap(
			&dx12Desc,
			IID_PPV_ARGS(&this->m_gpuTimestampQueryHeap)));
}

void phx::rhi::d3d12::D3D12GfxDevice::InitializeResourcePools()
{
	constexpr uint32_t kInitPoolSize = 100;
	this->m_texturePool.Initialize(kInitPoolSize);
	this->m_commandSignaturePool.Initialize(kInitPoolSize);
	this->m_shaderPool.Initialize(kInitPoolSize);
	this->m_inputLayoutPool.Initialize(kInitPoolSize);
	this->m_bufferPool.Initialize(kInitPoolSize);
	this->m_renderPassPool.Initialize(kInitPoolSize);
	this->m_rtAccelerationStructurePool.Initialize(kInitPoolSize);
	this->m_gfxPipelinePool.Initialize(kInitPoolSize);
	this->m_computePipelinePool.Initialize(kInitPoolSize);
	this->m_meshPipelinePool.Initialize(kInitPoolSize);
	this->m_timerQueryPool.Initialize(kInitPoolSize);
	this->m_commandListPool.Initialize(kInitPoolSize);
}

void phx::rhi::d3d12::D3D12GfxDevice::InitializeD3D12NativeResources(IDXGIAdapter* gpuAdapter)
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

	this->m_rootDevice->SetName(L"D3D12GfxDevice::RootDevice");
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
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(this->m_rootDevice->QueryInterface<ID3D12InfoQueue>(&infoQueue)))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);

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

void phx::rhi::d3d12::D3D12GfxDevice::CreateSwapChain(SwapChainDesc const& desc, void* windowHandle)
{
	this->m_swapChain.Desc = desc;

	HRESULT hr;

	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	const auto& formatMapping = GetDxgiFormatMapping(desc.Format);
	if (this->m_swapChain.NativeSwapchain == nullptr)
	{
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

		hr = this->m_factory->CreateSwapChainForHwnd(
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
}

void phx::rhi::d3d12::D3D12GfxDevice::FindAdapter(Microsoft::WRL::ComPtr<IDXGIFactory6> factory, D3D12Adapter& outAdapter)
{
	PHX_LOG_CORE_INFO("Finding a suitable adapter");

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

		std::string name = NarrowString(desc.Description);
		size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
		size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
		size_t sharedSystemMemory = desc.SharedSystemMemory;

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			PHX_LOG_CORE_INFO("GPU '%s' is a software adapter. Skipping consideration as this is not supported.", name.c_str());
			continue;
		}

		D3D12DeviceBasicInfo basicDeviceInfo = {};
		if (!SafeTestD3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_11_1, basicDeviceInfo))
		{
			continue;
		}

		if (basicDeviceInfo.NumDeviceNodes > 1)
		{
			PHX_LOG_CORE_INFO("GPU '%s' has one or more device nodes. Currently only support one device ndoe.", name.c_str());
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
		PHX_LOG_CORE_WARN("No suitable adapters were found.");
		return;
	}

	DXGI_ADAPTER_DESC desc = {};
	selectedAdapter->GetDesc(&desc);

	std::string name = NarrowString(desc.Description);
	size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
	size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
	size_t sharedSystemMemory = desc.SharedSystemMemory;

	// TODO: FIXLOG
	PHX_LOG_CORE_INFO(
		"Found Suitable D3D12 Adapter '%s'",
		name.c_str());

	// TODO: FIXLOG
	PHX_LOG_CORE_INFO(
		"Adapter has %dMB of dedicated video memory, %dMB of dedicated system memory, and %dMB of shared system memory.",
		dedicatedVideoMemory / (1024 * 1024),
		dedicatedSystemMemory / (1024 * 1024),
		sharedSystemMemory / (1024 * 1024));

	outAdapter.Name = NarrowString(desc.Description);
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

CommandListHandle phx::rhi::d3d12::D3D12GfxDevice::BeginCommandList(CommandQueueType queueType)
{
	uint32_t currentCmdIndex = this->m_activeCmdCount++;
	assert(currentCmdIndex < this->m_frameCommandListHandles.size());

	CommandListHandle cmdHandle = this->m_frameCommandListHandles[currentCmdIndex];
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmdHandle);
	internalCmd.NativeCommandAllocator = this->GetQueue(queueType).RequestAllocator();

	if (internalCmd.NativeCommandList == nullptr)
	{
		this->GetD3D12Device()->CreateCommandList(
			0,
			this->GetQueue(queueType).GetType(),
			internalCmd.NativeCommandAllocator,
			nullptr,
			IID_PPV_ARGS(&internalCmd.NativeCommandList));

		internalCmd.NativeCommandList->SetName(L"D3D12GfxDevice::CommandList");
		ThrowIfFailed(
			internalCmd.NativeCommandList.As<ID3D12GraphicsCommandList6>(&internalCmd.NativeCommandList6));

	}
	else
	{
		internalCmd.NativeCommandList->Reset(internalCmd.NativeCommandAllocator, nullptr);
	}


	internalCmd.Id = currentCmdIndex;
	internalCmd.QueueType = queueType;
	internalCmd.Waits.clear();
	internalCmd.IsWaitedOn.store(false);
	// internalCmd.UploadBuffer.Reset();

	// Bind Heaps
	std::array<ID3D12DescriptorHeap*, 2> heaps;
	for (int i = 0; i < this->m_gpuDescriptorHeaps.size(); i++)
	{
		heaps[i] = this->m_gpuDescriptorHeaps[i].GetNativeHeap();
	}

	// TODO: NOT VALID FOR COPY
	internalCmd.NativeCommandList6->SetDescriptorHeaps(heaps.size(), heaps.data());
	return cmdHandle;
}

void phx::rhi::d3d12::D3D12GfxDevice::WaitCommandList(CommandListHandle cmd, CommandListHandle waitOn)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	internalCmd.Waits.push_back(waitOn);
	internalCmd.IsWaitedOn.store(true);
}

void phx::rhi::d3d12::D3D12GfxDevice::RTBuildAccelerationStructure(RHI::RTAccelerationStructureHandle accelStructure, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12RTAccelerationStructure* dx12AccelStructure = this->GetRTAccelerationStructurePool().Get(accelStructure);
	assert(dx12AccelStructure);


	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC dx12BuildDesc = {};
	dx12BuildDesc.Inputs = dx12AccelStructure->Dx12Desc;

	internalCmd.BuildGeometries = dx12AccelStructure->Geometries;

	// Create a local copy.
	dx12BuildDesc.Inputs.pGeometryDescs = internalCmd.BuildGeometries.data();

	switch (dx12AccelStructure->Desc.Type)
	{
	case RTAccelerationStructureDesc::Type::BottomLevel:
	{
		for (int i = 0; i < dx12AccelStructure->Desc.ButtomLevel.Geometries.size(); i++)
		{
			auto& geometry = dx12AccelStructure->Desc.ButtomLevel.Geometries[i];
			auto& buildGeometry = internalCmd.BuildGeometries[i];

			if (geometry.Flags & RTAccelerationStructureDesc::BottomLevelDesc::Geometry::kOpaque)
			{
				buildGeometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
			}
			if (geometry.Flags & RTAccelerationStructureDesc::BottomLevelDesc::Geometry::kNoduplicateAnyHitInvocation)
			{
				buildGeometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
			}

			if (geometry.Type == RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::Triangles)
			{
				D3D12Buffer* vertexBuffer = this->GetBufferPool().Get(geometry.Triangles.VertexBuffer);
				assert(vertexBuffer);
				buildGeometry.Triangles.VertexBuffer.StartAddress = vertexBuffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)geometry.Triangles.VertexByteOffset;


				D3D12Buffer* indexBuffer = this->GetBufferPool().Get(geometry.Triangles.IndexBuffer);
				assert(indexBuffer);
				buildGeometry.Triangles.IndexBuffer = indexBuffer->D3D12Resource->GetGPUVirtualAddress() +
					(D3D12_GPU_VIRTUAL_ADDRESS)geometry.Triangles.IndexOffset * (geometry.Triangles.IndexFormat == RHI::Format::R16_UINT ? sizeof(uint16_t) : sizeof(uint32_t));

				if (geometry.Flags & RTAccelerationStructureDesc::BottomLevelDesc::Geometry::kUseTransform)
				{
					D3D12Buffer* transform3x4Buffer = this->GetBufferPool().Get(geometry.Triangles.Transform3x4Buffer);
					assert(transform3x4Buffer);
					buildGeometry.Triangles.Transform3x4 = transform3x4Buffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)geometry.Triangles.Transform3x4BufferOffset;
				}
			}
			else if (geometry.Type == RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::ProceduralAABB)
			{
				D3D12Buffer* aabbBuffer = this->GetBufferPool().Get(geometry.AABBs.AABBBuffer);
				assert(aabbBuffer);
				buildGeometry.AABBs.AABBs.StartAddress = aabbBuffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)geometry.AABBs.Offset;
			}
		}
		break;
	}
	case RTAccelerationStructureDesc::Type::TopLevel:
	{
		D3D12Buffer* instanceBuffer = this->GetBufferPool().Get(dx12AccelStructure->Desc.TopLevel.InstanceBuffer);
		assert(instanceBuffer);

		dx12BuildDesc.Inputs.InstanceDescs = instanceBuffer->D3D12Resource->GetGPUVirtualAddress() +
			(D3D12_GPU_VIRTUAL_ADDRESS)dx12AccelStructure->Desc.TopLevel.Offset;
		break;
	}
	default:
		return;
	}

	// TODO: handle Update
	/*
	if (src != nullptr)
	{
		desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

		auto src_internal = to_internal(src);
		desc.SourceAccelerationStructureData = src_internal->gpu_address;
	}
	*/
	dx12BuildDesc.DestAccelerationStructureData = dx12AccelStructure->D3D12Resource->GetGPUVirtualAddress();


	D3D12Buffer* scratchBuffer = this->GetBufferPool().Get(dx12AccelStructure->SratchBuffer);
	assert(scratchBuffer);

	dx12BuildDesc.ScratchAccelerationStructureData = scratchBuffer->D3D12Resource->GetGPUVirtualAddress();
	internalCmd.NativeCommandList6->BuildRaytracingAccelerationStructure(&dx12BuildDesc, 0, nullptr);
}

void D3D12GfxDevice::BeginMarker(std::string_view name, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	PIXBeginEvent(internalCmd.NativeCommandList.Get(), 0, std::wstring(name.begin(), name.end()).c_str());
	const char* tag = std::string(name.begin(), name.end()).c_str();
	// OPTICK_GPU_EVENT(tag);
}

void D3D12GfxDevice::EndMarker(CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	PIXEndEvent(internalCmd.NativeCommandList.Get());
}

void D3D12GfxDevice::TransitionBarrier(
	TextureHandle texture,
	ResourceStates beforeState,
	ResourceStates afterState,
	CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12Texture* textureImpl = this->GetTexturePool().Get(texture);
	assert(textureImpl);

	internalCmd.TransitionBarrier(
		textureImpl->D3D12Resource,
		ConvertResourceStates(beforeState),
		ConvertResourceStates(afterState));
}

void phx::rhi::d3d12::D3D12GfxDevice::TransitionBarrier(
	BufferHandle buffer,
	ResourceStates beforeState,
	ResourceStates afterState,
	CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12Buffer* bufferImpl = this->GetBufferPool().Get(buffer);
	assert(bufferImpl);

	internalCmd.TransitionBarrier(
		bufferImpl->D3D12Resource,
		ConvertResourceStates(beforeState),
		ConvertResourceStates(afterState));
}

void phx::rhi::d3d12::D3D12GfxDevice::TransitionBarriers(Span<GpuBarrier> gpuBarriers, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	for (const phx::rhi::GpuBarrier& gpuBarrier : gpuBarriers)
	{
		if (const GpuBarrier::TextureBarrier* texBarrier = std::get_if<GpuBarrier::TextureBarrier>(&gpuBarrier.Data))
		{
			D3D12Texture* textureImpl = this->GetTexturePool().Get(texBarrier->Texture);
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = textureImpl->D3D12Resource.Get();
			barrier.Transition.StateBefore = ConvertResourceStates(texBarrier->BeforeState);
			barrier.Transition.StateAfter = ConvertResourceStates(texBarrier->AfterState);
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			if (texBarrier->Mip >= 0 || texBarrier->Slice >= 0)
			{
				barrier.Transition.Subresource = D3D12CalcSubresource(
					(UINT)std::max(0, texBarrier->Mip),
					(UINT)std::max(0, texBarrier->Slice),
					0,
					textureImpl->Desc.MipLevels,
					textureImpl->Desc.ArraySize);
			}

			internalCmd.BarrierMemoryPool.push_back(barrier);
		}
		else if (const GpuBarrier::BufferBarrier* bufferBarrier = std::get_if<GpuBarrier::BufferBarrier>(&gpuBarrier.Data))
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource = this->GetBufferPool().Get(bufferBarrier->Buffer)->D3D12Resource;
			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				D3D12Resource.Get(),
				ConvertResourceStates(bufferBarrier->BeforeState),
				ConvertResourceStates(bufferBarrier->AfterState));

			internalCmd.BarrierMemoryPool.push_back(barrier);
		}

		else if (const GpuBarrier::MemoryBarrier* memoryBarrier = std::get_if<GpuBarrier::MemoryBarrier>(&gpuBarrier.Data))
		{
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.UAV.pResource = nullptr;

			if (const TextureHandle* texture = std::get_if<TextureHandle>(&memoryBarrier->Resource))
			{
				D3D12Texture* textureImpl = this->GetTexturePool().Get(*texture);
				if (textureImpl && textureImpl->D3D12Resource != nullptr)
				{
					barrier.UAV.pResource = textureImpl->D3D12Resource.Get();
				}
			}
			else if (const BufferHandle* buffer = std::get_if<BufferHandle>(&memoryBarrier->Resource))
			{
				D3D12Buffer* bufferImpl = this->GetBufferPool().Get(*buffer);
				if (bufferImpl && bufferImpl->D3D12Resource != nullptr)
				{
					barrier.UAV.pResource = bufferImpl->D3D12Resource.Get();
				}
			}

			internalCmd.BarrierMemoryPool.push_back(barrier);
		}
	}

	if (!internalCmd.BarrierMemoryPool.empty())
	{
		// TODO: Batch Barrier
		internalCmd.NativeCommandList->ResourceBarrier(
			internalCmd.BarrierMemoryPool.size(),
			internalCmd.BarrierMemoryPool.data());

		internalCmd.BarrierMemoryPool.clear();
	}
}

void D3D12GfxDevice::ClearTextureFloat(TextureHandle texture, Color const& clearColour, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	auto textureImpl = this->GetTexturePool().Get(texture);
	assert(textureImpl);

	assert(!textureImpl->RtvAllocation.Allocation.IsNull());
	internalCmd.NativeCommandList->ClearRenderTargetView(
		textureImpl->RtvAllocation.Allocation.GetCpuHandle(),
		&clearColour.R,
		0,
		nullptr);
}


GPUAllocation D3D12GfxDevice::AllocateGpu(size_t bufferSize, size_t stride, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	assert(bufferSize <= internalCmd.UploadBuffer.GetPageSize());

	auto heapAllocation = internalCmd.UploadBuffer.Allocate(bufferSize, stride);

	GPUAllocation gpuAlloc = {};
	gpuAlloc.GpuBuffer = heapAllocation.BufferHandle;
	gpuAlloc.CpuData = heapAllocation.CpuData;
	gpuAlloc.Offset = heapAllocation.Offset;
	gpuAlloc.SizeInBytes = bufferSize;

	return gpuAlloc;
}

void D3D12GfxDevice::ClearDepthStencilTexture(
	TextureHandle depthStencil,
	bool clearDepth,
	float depth,
	bool clearStencil,
	uint8_t stencil,
	CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	auto textureImpl = this->GetTexturePool().Get(depthStencil);
	assert(textureImpl);

	D3D12_CLEAR_FLAGS flags = {};
	if (clearDepth)
	{
		flags |= D3D12_CLEAR_FLAG_DEPTH;
	}

	if (clearStencil)
	{
		flags |= D3D12_CLEAR_FLAG_STENCIL;
	}

	assert(!textureImpl->DsvAllocation.Allocation.IsNull());
	internalCmd.NativeCommandList->ClearDepthStencilView(
		textureImpl->DsvAllocation.Allocation.GetCpuHandle(),
		flags,
		depth,
		stencil,
		0,
		nullptr);
}

void D3D12GfxDevice::Draw(DrawArgs const& args, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	internalCmd.NativeCommandList->DrawInstanced(
		args.VertexCount,
		args.InstanceCount,
		args.StartVertex,
		args.StartInstance);
}

void D3D12GfxDevice::DrawIndexed(DrawArgs const& args, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	internalCmd.NativeCommandList->DrawIndexedInstanced(
		args.IndexCount,
		args.InstanceCount,
		args.StartIndex,
		args.BaseVertex,
		args.StartInstance);
}

void phx::rhi::d3d12::D3D12GfxDevice::ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12CommandSignature* commandSignatureImpl = this->GetCommandSignaturePool().Get(commandSignature);
	D3D12Buffer* bufferImpl = this->GetBufferPool().Get(args);
	internalCmd.NativeCommandList6->ExecuteIndirect(
		commandSignatureImpl->NativeSignature.Get(),
		maxCount,
		bufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		nullptr,
		1);
}

void phx::rhi::d3d12::D3D12GfxDevice::ExecuteIndirect(
	RHI::CommandSignatureHandle commandSignature,
	RHI::BufferHandle args,
	size_t argsOffsetInBytes,
	RHI::BufferHandle count,
	size_t countOffsetInBytes,
	uint32_t maxCount,
	CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12CommandSignature* commandSignatureImpl = this->GetCommandSignaturePool().Get(commandSignature);
	D3D12Buffer* argBufferImpl = this->GetBufferPool().Get(args);
	D3D12Buffer* countBufferBufferImpl = this->GetBufferPool().Get(count);
	internalCmd.NativeCommandList6->ExecuteIndirect(
		commandSignatureImpl->NativeSignature.Get(),
		maxCount,
		argBufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		countBufferBufferImpl->D3D12Resource.Get(),
		countOffsetInBytes);
}

void phx::rhi::d3d12::D3D12GfxDevice::DrawIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12Buffer* bufferImpl = this->GetBufferPool().Get(args);
	internalCmd.NativeCommandList6->ExecuteIndirect(
		this->GetDrawInstancedIndirectCommandSignature(),
		maxCount,
		bufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		nullptr,
		1);
}

void phx::rhi::d3d12::D3D12GfxDevice::DrawIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12Buffer* argBufferImpl = this->GetBufferPool().Get(args);
	D3D12Buffer* countBufferBufferImpl = this->GetBufferPool().Get(args);
	internalCmd.NativeCommandList6->ExecuteIndirect(
		this->GetDrawInstancedIndirectCommandSignature(),
		maxCount,
		argBufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		countBufferBufferImpl->D3D12Resource.Get(),
		countOffsetInBytes);
}

void phx::rhi::d3d12::D3D12GfxDevice::DrawIndexedIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12Buffer* bufferImpl = this->GetBufferPool().Get(args);
	internalCmd.NativeCommandList6->ExecuteIndirect(
		this->GetDrawIndexedInstancedIndirectCommandSignature(),
		maxCount,
		bufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		nullptr,
		1);
}

void phx::rhi::d3d12::D3D12GfxDevice::DrawIndexedIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12Buffer* argBufferImpl = this->GetBufferPool().Get(args);
	D3D12Buffer* countBufferBufferImpl = this->GetBufferPool().Get(args);
	internalCmd.NativeCommandList6->ExecuteIndirect(
		this->GetDrawIndexedInstancedIndirectCommandSignature(),
		maxCount,
		argBufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		countBufferBufferImpl->D3D12Resource.Get(),
		countOffsetInBytes);
}

void D3D12GfxDevice::WriteBuffer(BufferHandle buffer, const void* Data, size_t dataSize, uint64_t destOffsetBytes, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	UINT64 alignedSize = dataSize;
	const D3D12Buffer* bufferImpl = this->GetBufferPool().Get(buffer);
	if ((bufferImpl->GetDesc().Binding & BindingFlags::ConstantBuffer) == BindingFlags::ConstantBuffer)
	{
		alignedSize = MemoryAlign(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	auto heapAllocation = internalCmd.UploadBuffer.Allocate(dataSize, alignedSize);
	D3D12Buffer* uploadBufferImpl = this->GetBufferPool().Get(heapAllocation.BufferHandle);
	memcpy(heapAllocation.CpuData, Data, dataSize);
	internalCmd.NativeCommandList->CopyBufferRegion(
		bufferImpl->D3D12Resource.Get(),
		destOffsetBytes,
		uploadBufferImpl->D3D12Resource.Get(),
		heapAllocation.Offset,
		dataSize);

	// TODO: See how the upload buffer can be used here
	/*
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
	ThrowIfFailed(
		this->GetD3D12Device2()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(dataSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&intermediateResource)));

	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = data;
	subresourceData.RowPitch = dataSize;
	subresourceData.SlicePitch = subresourceData.RowPitch;

	GpuBuffer* bufferImpl = SafeCast<GpuBuffer*>(buffer.Get());

	UpdateSubresources(
		internalCmd.NativeCommandList.Get(),
		bufferImpl->D3D12Resource,
		intermediateResource,
		0, 0, 1, &subresourceData);
		this->m_trackedData->NativeResources.push_back(intermediateResource);
	*/
}

void D3D12GfxDevice::CopyBuffer(BufferHandle dst, uint64_t dstOffset, BufferHandle src, uint64_t srcOffset, size_t sizeInBytes, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	const D3D12Buffer* srcBuffer = this->GetBufferPool().Get(src);
	const D3D12Buffer* dstBuffer = this->GetBufferPool().Get(dst);

	internalCmd.NativeCommandList->CopyBufferRegion(
		dstBuffer->D3D12Resource.Get(),
		(UINT64)dstOffset,
		srcBuffer->D3D12Resource.Get(),
		(UINT64)srcOffset,
		(UINT64)sizeInBytes);
}

void phx::rhi::d3d12::D3D12GfxDevice::WriteTexture(TextureHandle texture, uint32_t firstSubresource, size_t numSubresources, SubresourceData* pSubresourceData, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	auto textureImpl = this->GetTexturePool().Get(texture);
	UINT64 requiredSize = GetRequiredIntermediateSize(textureImpl->D3D12Resource.Get(), firstSubresource, numSubresources);

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
	ThrowIfFailed(
		this->GetD3D12Device2()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&intermediateResource)));


	std::vector<D3D12_SUBRESOURCE_DATA> subresources(numSubresources);
	for (int i = 0; i < numSubresources; ++i)
	{
		auto& subresource = subresources[i];
		subresource.RowPitch = pSubresourceData[i].rowPitch;
		subresource.SlicePitch = pSubresourceData[i].slicePitch;
		subresource.pData = pSubresourceData[i].pData;
	}

	UpdateSubresources(
		internalCmd.NativeCommandList.Get(),
		textureImpl->D3D12Resource.Get(),
		intermediateResource.Get(),
		0,
		firstSubresource,
		subresources.size(),
		subresources.data());

	internalCmd.TrackedResources.push_back(intermediateResource);
}

void phx::rhi::d3d12::D3D12GfxDevice::WriteTexture(TextureHandle texture, uint32_t arraySlice, uint32_t mipLevel, const void* Data, size_t rowPitch, size_t depthPitch, CommandListHandle cmd)
{
	// LOG_CORE_FATAL("NOT IMPLEMENTED FUNCTION CALLED: CommandList::WriteTexture");
	assert(false);
}

void phx::rhi::d3d12::D3D12GfxDevice::SetRenderTargets(Span<TextureHandle> renderTargets, TextureHandle depthStencil, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetViews(renderTargets.Size());
	for (int i = 0; i < renderTargets.Size(); i++)
	{
		auto textureImpl = this->GetTexturePool().Get(renderTargets[i]);
		renderTargetViews[i] = textureImpl->RtvAllocation.Allocation.GetCpuHandle();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE depthView = {};
	bool hasDepth = depthStencil.IsValid();
	if (hasDepth)
	{
		auto textureImpl = this->GetTexturePool().Get(depthStencil);
		depthView = textureImpl->DsvAllocation.Allocation.GetCpuHandle();
	}

	internalCmd.NativeCommandList->OMSetRenderTargets(
		renderTargetViews.size(),
		renderTargetViews.data(),
		hasDepth,
		hasDepth ? &depthView : nullptr);
}

void phx::rhi::d3d12::D3D12GfxDevice::SetGfxPipeline(GfxPipelineHandle graphicsPiplineHandle, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	internalCmd.ActivePipelineType = D3D12CommandList::D3D12CommandList::PipelineType::Gfx;
	D3D12GraphicsPipeline* graphisPipeline = this->GetGraphicsPipelinePool().Get(graphicsPiplineHandle);
	internalCmd.NativeCommandList->SetPipelineState(graphisPipeline->D3D12PipelineState.Get());

	internalCmd.NativeCommandList->SetGraphicsRootSignature(graphisPipeline->RootSignature.Get());

	const auto& desc = graphisPipeline->Desc;
	D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	switch (desc.PrimType)
	{
	case PrimitiveType::TriangleList:
		topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	case PrimitiveType::TriangleStrip:
		topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		break;
	case PrimitiveType::LineList:
		topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	default:
		assert(false);
	}
	internalCmd.NativeCommandList->IASetPrimitiveTopology(topology);
}

void phx::rhi::d3d12::D3D12GfxDevice::SetViewports(Viewport* viewports, size_t numViewports, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	CD3DX12_VIEWPORT dx12Viewports[16] = {};
	for (int i = 0; i < numViewports; i++)
	{
		Viewport& viewport = viewports[i];
		dx12Viewports[i] = CD3DX12_VIEWPORT(
			viewport.MinX,
			viewport.MinY,
			viewport.GetWidth(),
			viewport.GetHeight(),
			viewport.MinZ,
			viewport.MaxZ);
	}

	internalCmd.NativeCommandList->RSSetViewports((UINT)numViewports, dx12Viewports);
}

void phx::rhi::d3d12::D3D12GfxDevice::SetScissors(Rect* scissors, size_t numScissors, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	CD3DX12_RECT dx12Scissors[16] = {};
	for (int i = 0; i < numScissors; i++)
	{
		const Rect& scissor = scissors[i];

		dx12Scissors[i] = CD3DX12_RECT(
			scissor.MinX,
			scissor.MinY,
			scissor.MaxX,
			scissor.MaxY);
	}

	internalCmd.NativeCommandList->RSSetScissorRects((UINT)numScissors, dx12Scissors);
}

void phx::rhi::d3d12::D3D12GfxDevice::BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	if (internalCmd.ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		internalCmd.NativeCommandList->SetComputeRoot32BitConstants(rootParameterIndex, sizeInBytes / sizeof(uint32_t), constants, 0);
	}
	else
	{
		internalCmd.NativeCommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, sizeInBytes / sizeof(uint32_t), constants, 0);
	}
}

void phx::rhi::d3d12::D3D12GfxDevice::BindConstantBuffer(size_t rootParameterIndex, BufferHandle constantBuffer, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	const D3D12Buffer* constantBufferImpl = this->GetBufferPool().Get(constantBuffer);
	if (internalCmd.ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		internalCmd.NativeCommandList->SetComputeRootConstantBufferView(rootParameterIndex, constantBufferImpl->D3D12Resource->GetGPUVirtualAddress());
	}
	else
	{
		internalCmd.NativeCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, constantBufferImpl->D3D12Resource->GetGPUVirtualAddress());
	}
}

void phx::rhi::d3d12::D3D12GfxDevice::BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	UploadAllocation alloc = internalCmd.UploadBuffer.Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	std::memcpy(alloc.CpuData, bufferData, sizeInBytes);

	if (internalCmd.ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		internalCmd.NativeCommandList->SetComputeRootConstantBufferView(rootParameterIndex, alloc.Gpu);
	}
	else
	{
		internalCmd.NativeCommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, alloc.Gpu);
	}
}

void phx::rhi::d3d12::D3D12GfxDevice::BindVertexBuffer(uint32_t slot, BufferHandle vertexBuffer, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	const D3D12Buffer* vertexBufferImpl = this->GetBufferPool().Get(vertexBuffer);

	internalCmd.NativeCommandList->IASetVertexBuffers(slot, 1, &vertexBufferImpl->VertexView);
}

void phx::rhi::d3d12::D3D12GfxDevice::BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	size_t bufferSize = numVertices * vertexSize;
	auto heapAllocation = internalCmd.UploadBuffer.Allocate(bufferSize, vertexSize);
	memcpy(heapAllocation.CpuData, vertexBufferData, bufferSize);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = heapAllocation.Gpu;
	vertexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
	vertexBufferView.StrideInBytes = static_cast<UINT>(vertexSize);

	internalCmd.NativeCommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void phx::rhi::d3d12::D3D12GfxDevice::BindIndexBuffer(BufferHandle indexBuffer, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	const D3D12Buffer* bufferImpl = this->GetBufferPool().Get(indexBuffer);

	internalCmd.NativeCommandList->IASetIndexBuffer(&bufferImpl->IndexView);
}

void phx::rhi::d3d12::D3D12GfxDevice::BindDynamicIndexBuffer(size_t numIndicies, RHI::Format indexFormat, const void* indexBufferData, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	size_t indexSizeInBytes = indexFormat == RHI::Format::R16_UINT ? 2 : 4;
	size_t bufferSize = numIndicies * indexSizeInBytes;

	auto heapAllocation = internalCmd.UploadBuffer.Allocate(bufferSize, indexSizeInBytes);
	memcpy(heapAllocation.CpuData, indexBufferData, bufferSize);

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = heapAllocation.Gpu;
	indexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
	const auto& formatMapping = GetDxgiFormatMapping(indexFormat);;

	indexBufferView.Format = formatMapping.SrvFormat;

	internalCmd.NativeCommandList->IASetIndexBuffer(&indexBufferView);
}

void phx::rhi::d3d12::D3D12GfxDevice::BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	size_t sizeInBytes = numElements * elementSize;
	UploadAllocation alloc = internalCmd.UploadBuffer.Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	std::memcpy(alloc.CpuData, bufferData, sizeInBytes);

	if (internalCmd.ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		internalCmd.NativeCommandList->SetComputeRootShaderResourceView(rootParameterIndex, alloc.Gpu);
	}
	else
	{
		internalCmd.NativeCommandList->SetGraphicsRootShaderResourceView(rootParameterIndex, alloc.Gpu);
	}
}

void phx::rhi::d3d12::D3D12GfxDevice::BindStructuredBuffer(size_t rootParameterIndex, BufferHandle buffer, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	const D3D12Buffer* bufferImpl = this->GetBufferPool().Get(buffer);

	if (internalCmd.ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		internalCmd.NativeCommandList->SetComputeRootShaderResourceView(
			rootParameterIndex,
			bufferImpl->D3D12Resource->GetGPUVirtualAddress());
	}
	else
	{
		internalCmd.NativeCommandList->SetGraphicsRootShaderResourceView(
			rootParameterIndex,
			bufferImpl->D3D12Resource->GetGPUVirtualAddress());
	}
}

void phx::rhi::d3d12::D3D12GfxDevice::BindResourceTable(size_t rootParameterIndex, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	if (this->GetMinShaderModel() < ShaderModel::SM_6_6)
	{
		if (internalCmd.ActivePipelineType == D3D12CommandList::PipelineType::Compute)
		{
			internalCmd.NativeCommandList->SetComputeRootDescriptorTable(
				rootParameterIndex,
				this->m_bindlessResourceDescriptorTable.GetGpuHandle(0));
		}
		else
		{
			internalCmd.NativeCommandList->SetGraphicsRootDescriptorTable(
				rootParameterIndex,
				this->m_bindlessResourceDescriptorTable.GetGpuHandle(0));
		}
	}
}

void phx::rhi::d3d12::D3D12GfxDevice::BindSamplerTable(size_t rootParameterIndex, CommandListHandle cmd)
{
	// TODO:
	assert(false);
}

void D3D12GfxDevice::BindDynamicDescriptorTable(size_t rootParameterIndex, Span<TextureHandle> textures, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	// Request Descriptoprs for table
	// Validate with Root Signature. Maybe an improvment in the future.
	DescriptorHeapAllocation descriptorTable = internalCmd.ActiveDynamicSubAllocator->Allocate(textures.Size());
	for (int i = 0; i < textures.Size(); i++)
	{
		auto textureImpl = this->GetTexturePool().Get(textures[i]);
		this->GetD3D12Device2()->CopyDescriptorsSimple(
			1,
			descriptorTable.GetCpuHandle(i),
			textureImpl->Srv.Allocation.GetCpuHandle(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (internalCmd.ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		internalCmd.NativeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
	}
	else
	{
		internalCmd.NativeCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
	}
}

void phx::rhi::d3d12::D3D12GfxDevice::BindDynamicUavDescriptorTable(
	size_t rootParameterIndex,
	Span<BufferHandle> buffers,
	Span<TextureHandle> textures,
	CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	// Request Descriptoprs for table
	// Validate with Root Signature. Maybe an improvment in the future.
	DescriptorHeapAllocation descriptorTable = internalCmd.ActiveDynamicSubAllocator->Allocate(buffers.Size() + textures.Size());
	for (int i = 0; i < buffers.Size(); i++)
	{
		auto impl = this->GetBufferPool().Get(buffers[i]);
		this->GetD3D12Device2()->CopyDescriptorsSimple(
			1,
			descriptorTable.GetCpuHandle(i),
			impl->UavAllocation.Allocation.GetCpuHandle(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	}

	for (int i = 0; i < textures.Size(); i++)
	{
		auto textureImpl = this->GetTexturePool().Get(textures[i]);
		this->GetD3D12Device2()->CopyDescriptorsSimple(
			1,
			descriptorTable.GetCpuHandle(i + buffers.Size()),
			textureImpl->UavAllocation.Allocation.GetCpuHandle(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	}

	if (internalCmd.ActivePipelineType == D3D12CommandList::PipelineType::Compute)
	{
		internalCmd.NativeCommandList->SetComputeRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
	}
	else
	{
		internalCmd.NativeCommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, descriptorTable.GetGpuHandle());
	}
}

void phx::rhi::d3d12::D3D12GfxDevice::SetComputeState(ComputePipelineHandle state, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	internalCmd.ActivePipelineType = D3D12CommandList::PipelineType::Compute;
	D3D12ComputePipeline* computePsoImpl = this->GetComputePipelinePool().Get(state);
	internalCmd.NativeCommandList->SetComputeRootSignature(computePsoImpl->RootSignature.Get());
	internalCmd.NativeCommandList->SetPipelineState(computePsoImpl->D3D12PipelineState.Get());

	internalCmd.ActiveComputePipeline = computePsoImpl;
}

void phx::rhi::d3d12::D3D12GfxDevice::Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	internalCmd.NativeCommandList->Dispatch(groupsX, groupsY, groupsZ);
}

void phx::rhi::d3d12::D3D12GfxDevice::DispatchIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12Buffer* bufferImpl = this->GetBufferPool().Get(args);
	internalCmd.NativeCommandList6->ExecuteIndirect(
		this->GetDispatchIndirectCommandSignature(),
		maxCount,
		bufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		nullptr,
		1);
}

void phx::rhi::d3d12::D3D12GfxDevice::DispatchIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12Buffer* argBufferImpl = this->GetBufferPool().Get(args);
	D3D12Buffer* countBufferBufferImpl = this->GetBufferPool().Get(args);
	internalCmd.NativeCommandList6->ExecuteIndirect(
		this->GetDispatchIndirectCommandSignature(),
		maxCount,
		argBufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		countBufferBufferImpl->D3D12Resource.Get(),
		countOffsetInBytes);
}

void phx::rhi::d3d12::D3D12GfxDevice::SetMeshPipeline(MeshPipelineHandle meshPipeline, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	internalCmd.ActivePipelineType = D3D12CommandList::PipelineType::Mesh;
	internalCmd.ActiveMeshPipeline = this->GetMeshPipelinePool().Get(meshPipeline);
	internalCmd.NativeCommandList->SetPipelineState(internalCmd.ActiveMeshPipeline->D3D12PipelineState.Get());

	internalCmd.NativeCommandList->SetGraphicsRootSignature(internalCmd.ActiveMeshPipeline->RootSignature.Get());

	const auto& desc = internalCmd.ActiveMeshPipeline->Desc;
	D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	switch (desc.PrimType)
	{
	case PrimitiveType::TriangleList:
		topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	case PrimitiveType::TriangleStrip:
		topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		break;
	case PrimitiveType::LineList:
		topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		break;
	default:
		assert(false);
	}
	internalCmd.NativeCommandList->IASetPrimitiveTopology(topology);

	// TODO: Move viewport logic here as well...
}

void phx::rhi::d3d12::D3D12GfxDevice::DispatchMesh(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	assert(internalCmd.ActivePipelineType == D3D12CommandList::PipelineType::Mesh);
	internalCmd.NativeCommandList6->DispatchMesh(groupsX, groupsY, groupsZ);
}

void phx::rhi::d3d12::D3D12GfxDevice::DispatchMeshIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12Buffer* bufferImpl = this->GetBufferPool().Get(args);
	internalCmd.NativeCommandList6->ExecuteIndirect(
		this->GetDispatchIndirectCommandSignature(),
		maxCount,
		bufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		nullptr,
		1);
}

void phx::rhi::d3d12::D3D12GfxDevice::DispatchMeshIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12Buffer* argBufferImpl = this->GetBufferPool().Get(args);
	D3D12Buffer* countBufferBufferImpl = this->GetBufferPool().Get(args);
	internalCmd.NativeCommandList6->ExecuteIndirect(
		this->GetDispatchIndirectCommandSignature(),
		maxCount,
		argBufferImpl->D3D12Resource.Get(),
		argsOffsetInBytes,
		countBufferBufferImpl->D3D12Resource.Get(),
		countOffsetInBytes);
}

void phx::rhi::d3d12::D3D12GfxDevice::BeginTimerQuery(TimerQueryHandle query, CommandListHandle cmd)
{
	assert(query.IsValid());
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12TimerQuery* queryImpl = this->GetTimerQueryPool().Get(query);

	internalCmd.TimerQueries.push_back(query);
	internalCmd.NativeCommandList->EndQuery(
		this->GetQueryHeap(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		queryImpl->BeginQueryIndex);
}

void phx::rhi::d3d12::D3D12GfxDevice::EndTimerQuery(TimerQueryHandle query, CommandListHandle cmd)
{
	D3D12CommandList& internalCmd = *this->m_commandListPool.Get(cmd);
	D3D12TimerQuery* queryImpl = this->GetTimerQueryPool().Get(query);

	internalCmd.TimerQueries.push_back(query);

	const D3D12Buffer* timeStampQueryBuffer = this->GetBufferPool().Get(this->GetTimestampQueryBuffer());

	internalCmd.NativeCommandList->EndQuery(
		this->GetQueryHeap(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		queryImpl->EndQueryIndex);

	internalCmd.NativeCommandList->ResolveQueryData(
		this->GetQueryHeap(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		queryImpl->BeginQueryIndex,
		2,
		timeStampQueryBuffer->D3D12Resource.Get(),
		queryImpl->BeginQueryIndex * sizeof(uint64_t));
}
