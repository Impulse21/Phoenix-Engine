#include "pch.h"

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