#include "pch.h"

#include "phx/core/Math.h"
#include "phx/core/CommandLineArgs.h"

#include "Dx12GfxDevice.h"
#include "phx/core/Log.h"
#include "phx/core/StringUtils.h"

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::dx12;


extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

#ifdef USING_D3D12_AGILITY_SDK
	// Used to enable the "Agility SDK" components
	__declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
	__declspec(dllexport) extern const char8_t* D3D12SDKPath = u8".\\D3D12\\";
#endif
}

// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2

// Shader reflecting sample
#if false

#include <dxcapi.h>
#include <d3d12shader.h> // Contains functions and structures useful in accessing shader information.
Microsoft::WRL::ComPtr<ID3D12ShaderReflection> shaderReflection{};
utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection));
D3D12_SHADER_DESC shaderDesc{};
shaderReflection->GetDesc(&shaderDesc);

uint16_t stage = (shaderDesc.Version & 0xFFFF0000) >> 16;  // Extract the stage bits
uint16_t major = (shaderDesc.Version & 0x000000F0) >> 4;
uint16_t minor = (shaderDesc.Version & 0x0000000F);

assert(D3D12_SHVER_VERTEX_SHADER == stage);

#endif

namespace
{

	D3D12_SHADER_VISIBILITY ConvertShaderStage(ShaderStage s)
	{
		switch (s)  // NOLINT(clang-diagnostic-switch-enum)
		{
		case ShaderStage::VS:
			return D3D12_SHADER_VISIBILITY_VERTEX;
		case ShaderStage::HS:
			return D3D12_SHADER_VISIBILITY_HULL;
		case ShaderStage::DS:
			return D3D12_SHADER_VISIBILITY_DOMAIN;
		case ShaderStage::GS:
			return D3D12_SHADER_VISIBILITY_GEOMETRY;
		case ShaderStage::PS:
			return D3D12_SHADER_VISIBILITY_PIXEL;
		case ShaderStage::AS:
			return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
		case ShaderStage::MS:
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

	void PollDebugMessages(ID3D12Device* device)
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
		if (FAILED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			return;
		}

		const UINT64 messageCount = infoQueue->GetNumStoredMessages();

		for (UINT64 i = 0; i < messageCount; i++)
		{
			SIZE_T messageLength = 0;
			infoQueue->GetMessage(i, nullptr, &messageLength);

			std::vector<char> messageData(messageLength);
			D3D12_MESSAGE* message = reinterpret_cast<D3D12_MESSAGE*>(messageData.data());
			infoQueue->GetMessage(i, message, &messageLength);

			PHX_CORE_INFO("[DX12Driver] - {0}", message->pDescription);
		}

		infoQueue->ClearStoredMessages();
	}

	Microsoft::WRL::ComPtr<IDXGIFactory6> CreateDXGIFactory6(bool enableDebugLayers)
	{
		uint32_t flags = 0;
		if (enableDebugLayers)
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
				PHX_CORE_WARN("D3D12CreateDevice failed.");
			}
		}
		catch (...)
		{
		}
#pragma warning(default:6322)

		return false;
	}

	void FindAdapter(Microsoft::WRL::ComPtr<IDXGIFactory6> factory, D3D12Adapter& outAdapter)
	{
		PHX_CORE_INFO("Finding a suitable adapter");

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
			phx::StringConvert(desc.Description, name);
			size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
			// size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
			// size_t sharedSystemMemory = desc.SharedSystemMemory;

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				PHX_CORE_INFO("GPU '{0}' is a software adapter. Skipping consideration as this is not supported.", name.c_str());
				continue;
			}

			D3D12DeviceBasicInfo basicDeviceInfo = {};
			if (!SafeTestD3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_11_1, basicDeviceInfo))
			{
				continue;
			}

			if (basicDeviceInfo.NumDeviceNodes > 1)
			{
				PHX_CORE_INFO("GPU {0}' has one or more device nodes. Currently only support one device ndoe.", name.c_str());
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
			PHX_CORE_WARN("No suitable adapters were found.");
			return;
		}

		DXGI_ADAPTER_DESC desc = {};
		selectedAdapter->GetDesc(&desc);

		std::string name;
		phx::StringConvert(desc.Description, name);
		size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
		size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
		size_t sharedSystemMemory = desc.SharedSystemMemory;

		// TODO: FIXLOG
		PHX_CORE_INFO(
			"Found Suitable D3D12 Adapter '{0}'",
			name.c_str());

		// TODO: FIXLOG
		PHX_CORE_INFO(
			"Adapter has {0}MB of dedicated video memory, {1}MB of dedicated system memory, and {2}MB of shared system memory.",
			dedicatedVideoMemory / (1024 * 1024),
			dedicatedSystemMemory / (1024 * 1024),
			sharedSystemMemory / (1024 * 1024));

		phx::StringConvert(desc.Description, outAdapter.Name);
		outAdapter.BasicDeviceInfo = selectedBasicDeviceInfo;
		outAdapter.NativeDesc = desc;
		outAdapter.NativeAdapter = selectedAdapter;
	}

	void TranslateBlendState(BlendRenderState const& inState, D3D12_BLEND_DESC& outState)
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

	void TranslateDepthStencilState(DepthStencilRenderState const& inState, D3D12_DEPTH_STENCIL_DESC& outState)
	{
		outState.DepthEnable = inState.DepthEnable ? TRUE : FALSE;
		outState.DepthWriteMask = inState.DepthWriteMask == DepthWriteMask::All ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		outState.DepthFunc = ConvertComparisonFunc(inState.DepthFunc);
		outState.StencilEnable = inState.StencilEnable ? TRUE : FALSE;
		outState.StencilReadMask = (UINT8)inState.StencilReadMask;
		outState.StencilWriteMask = (UINT8)inState.StencilWriteMask;
		outState.FrontFace.StencilFailOp = ConvertStencilOp(inState.FrontFace.StencilFailOp);
		outState.FrontFace.StencilDepthFailOp = ConvertStencilOp(inState.FrontFace.StencilDepthFailOp);
		outState.FrontFace.StencilPassOp = ConvertStencilOp(inState.FrontFace.StencilPassOp);
		outState.FrontFace.StencilFunc = ConvertComparisonFunc(inState.FrontFace.StencilFunc);
		outState.BackFace.StencilFailOp = ConvertStencilOp(inState.BackFace.StencilFailOp);
		outState.BackFace.StencilDepthFailOp = ConvertStencilOp(inState.BackFace.StencilDepthFailOp);
		outState.BackFace.StencilPassOp = ConvertStencilOp(inState.BackFace.StencilPassOp);
		outState.BackFace.StencilFunc = ConvertComparisonFunc(inState.BackFace.StencilFunc);
	}

	void TranslateRasterState(RasterRenderState const& inState, D3D12_RASTERIZER_DESC& outState)
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

	CompPtr<ID3D12RootSignature> CreateRootSignature(phx::Span<uint8_t> byteCode)
	{
		HRESULT hr = (byteCode.IsEmpty() ? E_FAIL : S_OK);
		assert(SUCCEEDED(hr));

		CompPtr<ID3D12RootSignature> rootSig;
		CompPtr<ID3D12VersionedRootSignatureDeserializer> rootsigDeserializer;
		hr = D3D12CreateVersionedRootSignatureDeserializer(
			byteCode.data(),
			byteCode.size(),
			IID_PPV_ARGS(rootsigDeserializer.ReleaseAndGetAddressOf()));

		if (SUCCEEDED(hr))
		{
			const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* rootsigDesc = nullptr;
			hr = rootsigDeserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1, &rootsigDesc);
			if (SUCCEEDED(hr))
			{
				assert(rootsigDesc->Version == D3D_ROOT_SIGNATURE_VERSION_1_1);

				hr = GfxDeviceDx12::Instance()->GetD3D12Device2()->CreateRootSignature(
					0,
					byteCode.data(),
					byteCode.size(),
					IID_PPV_ARGS(rootSig.ReleaseAndGetAddressOf())
				);
				assert(SUCCEEDED(hr));
			}
		}

		return rootSig;
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateEmptyRootSignature()
	{
		using namespace Microsoft::WRL;

		// Define an empty root signature
		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = 0;         // No root parameters
		rootSignatureDesc.pParameters = nullptr;
		rootSignatureDesc.NumStaticSamplers = 0;     // No static samplers
		rootSignatureDesc.pStaticSamplers = nullptr;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// Serialize the root signature
		ComPtr<ID3DBlob> serializedRootSignature;
		ComPtr<ID3DBlob> errorBlob; // To capture any errors
		HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSignature,
			&errorBlob);

		if (FAILED(hr)) {
			if (errorBlob) {
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			}
			throw std::runtime_error("Failed to serialize root signature");
		}

		// Create the root signature
		ComPtr<ID3D12RootSignature> rootSignature;
		hr = GfxDeviceDx12::Instance()->GetD3D12Device()->CreateRootSignature(
			0,
			serializedRootSignature->GetBufferPointer(),
			serializedRootSignature->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature));

		if (FAILED(hr)) {
			throw std::runtime_error("Failed to create root signature");
		}

		return rootSignature;
	}
}

void GfxDeviceDx12::CopyCtxManager::Initialize()
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(
		GfxDeviceDx12::Instance()->GetD3D12Device()->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_copyQueue)));

	ThrowIfFailed(
		m_copyQueue->SetName(L"Copy Ctx Manager"));

}

void GfxDeviceDx12::CopyCtxManager::Finalize()
{
	// for (auto& ctx : m_freeList)
		// GfxDeviceDx12::Instance()->DeleteBuffer(ctx.UploadBuffer);
}

GfxDeviceDx12::CopyCtxManager::Ctx GfxDeviceDx12::CopyCtxManager::Begin(size_t stagingSize)
{
	Ctx retVal;
	{
		std::scoped_lock _(m_mutex);

		for (size_t i = 0; i < m_freeList.size(); i++)
		{
			Ctx& ctx = m_freeList[i];
			if (ctx.UploadBufferSize > stagingSize)
				continue;

			if (!ctx.IsCompleted())
				continue;

			retVal = ctx;
			std::swap(m_freeList[i], m_freeList.back());
			m_freeList.pop_back();
		}
	}

	if (retVal.IsInValid())
	{
		HRESULT hr = GfxDeviceDx12::Instance()->GetD3D12Device2()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&retVal.Allocator));
		assert(SUCCEEDED(hr));

		hr = GfxDeviceDx12::Instance()->GetD3D12Device2()->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_COPY,
			retVal.Allocator.Get(),
			nullptr,
			IID_PPV_ARGS(&retVal.CommandList));
		assert(SUCCEEDED(hr));

		retVal.CommandList->Close();

		hr = GfxDeviceDx12::Instance()->GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&retVal.Fence));
		assert(SUCCEEDED(hr));

		retVal.UploadBufferSize = math::GetNextPowerOfTwo(stagingSize);
#if false
		retVal.UploadBuffer = GfxDeviceDx12::Instance()->CreateBuffer({
			.Usage = Usage::Upload,
			.SizeInBytes = retVal.UploadBufferSize,
			.DebugName = "Upload Buffer"
			});

		D3D12Buffer* bufferImpl = GfxDeviceDx12::Instance()->GetRegistry().Buffers.Get(retVal.UploadBuffer);

		retVal.MappedData = bufferImpl->MappedData; 
#endif
	}

	// begin command list in valid state:
	HRESULT hr = retVal.Allocator->Reset();
	assert(SUCCEEDED(hr));
	hr = retVal.CommandList->Reset(retVal.Allocator.Get(), nullptr);
	assert(SUCCEEDED(hr));

	return retVal;
}

void GfxDeviceDx12::CopyCtxManager::Submit(Ctx ctx)
{
	HRESULT hr;

	{
		std::scoped_lock _(m_mutex);
		ctx.FenceValue++;
		m_freeList.push_back(ctx);
	}

	ctx.CommandList->Close();
	ID3D12CommandList* commandlists[] = {
		ctx.CommandList.Get()
	};

	m_copyQueue->ExecuteCommandLists(1, commandlists);
	hr = m_copyQueue->Signal(ctx.Fence.Get(), ctx.FenceValue);
	assert(SUCCEEDED(hr));

	hr = GfxDeviceDx12::Instance()->GetGfxQueue().Queue->Wait(ctx.Fence.Get(), ctx.FenceValue);
	assert(SUCCEEDED(hr));

	hr = GfxDeviceDx12::Instance()->GetComputeQueue().Queue->Wait(ctx.Fence.Get(), ctx.FenceValue);
	assert(SUCCEEDED(hr));

	hr = GfxDeviceDx12::Instance()->GetCopyQueue().Queue->Wait(ctx.Fence.Get(), ctx.FenceValue);
	assert(SUCCEEDED(hr));
}


GfxDeviceDx12::GfxDeviceDx12(GfxDeviceDescriptor const& descriptor)
	: m_descriptor(descriptor)
{
	assert(Singleton == nullptr);
	Singleton = this;

	Initialize();
	InitializeResourcePools();
	m_gpuTimerManager.Initialize();

	m_emptyRootSignature = CreateEmptyRootSignature();
	m_copyCtxManager.Initialize();
}

GfxDeviceDx12::~GfxDeviceDx12()
{
	WaitForIdle();

	// m_tempPageAllocator.Finalize();
	m_copyCtxManager.Finalize();

	FinalizeResourcePools();

	assert(Singleton);
	Singleton = nullptr;
}

void GfxDeviceDx12::WaitForIdle()
{
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	HRESULT hr = GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
	{
		D3D12CommandQueue& queue = m_commandQueues[q];
		hr = queue.Queue->Signal(fence.Get(), 1);
		assert(SUCCEEDED(hr));
		if (fence->GetCompletedValue() < 1)
		{
			hr = fence->SetEventOnCompletion(1, NULL);
			assert(SUCCEEDED(hr));
		}
		fence->Signal(0);
	}
}

#if false
void GfxDeviceDx12::ResizeSwapChain(SwapChainDesc const& swapChainDesc)
{
	CreateSwapChain(swapChainDesc, nullptr);
}
#endif
bool GfxDeviceDx12::CreateSwapChain(SwapChainDescriptor const& desc, SwapChain_Hot& hot, SwapChain_Cold& cold)
{
	HRESULT hr;

	cold.ClearColour = desc.OptmizedClearValue;
	cold.VSync = desc.VSync;
	cold.Fullscreen = desc.Fullscreen;
	cold.EnableHDR = desc.EnableHDR;

	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	const auto& formatMapping = GetDxgiFormatMapping(desc.Format);
	if (cold.SwapChain == nullptr)
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
		swapChainDesc.BufferCount = kBufferCount;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags = swapChainFlags;

		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
		fullscreenDesc.Windowed = !desc.Fullscreen;

		hr = m_factory->CreateSwapChainForHwnd(
			GetGfxQueue().Queue.Get(),
			static_cast<HWND>(desc.WindowHandle),
			&swapChainDesc,
			&fullscreenDesc,
			nullptr,
			cold.SwapChain.GetAddressOf()
		);

		if (FAILED(hr))
		{
			throw std::exception();
		}

		hr = cold.SwapChain.As(&cold.SwapChain4);
		if (FAILED(hr))
		{
			throw std::exception();
		}
	}
	else
	{
		// Resize swapchain:
		WaitForIdle();

		hot.CurrentBackBuffer = nullptr;
		hot.CurrentRtv = {};

		// Delete back buffers
		cold.ViewAllocation.Free();
		for (auto& backBuffer : cold.BackBuffers)
		{
			backBuffer.Reset();
		}

		hr = cold.SwapChain->ResizeBuffers(
			kBufferCount,
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

	cold.ViewAllocation = m_cpuDescriptorHeaps[DescriptorHeapTypes::RTV].Allocate(kBufferCount);
	for (UINT i = 0; i < kBufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource>& backBuffer = cold.BackBuffers[i];
		ThrowIfFailed(
			cold.SwapChain4->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		char allocatorName[32];
		sprintf_s(allocatorName, "Back Buffer %iu", i);

		GetD3D12Device()->CreateRenderTargetView(backBuffer.Get(), nullptr, cold.ViewAllocation.GetCpuHandle(i));
	}

	const UINT currentIndex = cold.SwapChain4->GetCurrentBackBufferIndex();
	hot.CurrentBackBuffer = cold.BackBuffers[currentIndex].Get();
	hot.CurrentRtv = cold.ViewAllocation.GetCpuHandle(currentIndex);

	return true;
}

bool GfxDeviceDx12::CreatePipeline(PipelineStateDescriptor const& desc, PipelineState_Hot& hot, PipelineState_Cold& cold)
{
	struct PSO_STREAM
	{
		struct PSO_STREAM1
		{
			CD3DX12_PIPELINE_STATE_STREAM_VS VS;
			CD3DX12_PIPELINE_STATE_STREAM_HS HS;
			CD3DX12_PIPELINE_STATE_STREAM_DS DS;
			CD3DX12_PIPELINE_STATE_STREAM_GS GS;
			CD3DX12_PIPELINE_STATE_STREAM_PS PS;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RS;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DSS;
			CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BD;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PT;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT IL;
			CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE STRIP;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS Formats;
			CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
			CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK SampleMask;
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE ROOTSIG;
		} stream1 = {};

		struct PSO_STREAM2
		{
			CD3DX12_PIPELINE_STATE_STREAM_MS MS;
			CD3DX12_PIPELINE_STATE_STREAM_AS AS;
		} stream2 = {};
	} stream = {};

	// TODO: Cache reflection data in some way.
	if (desc.MS.IsValid())
	{
		stream.stream2.MS = { desc.MS.ByteCode.data(), desc.MS.ByteCode.size() };
		if (cold.RootSignature == nullptr)
		{
			cold.RootSignature = CreateRootSignature(desc.MS.ByteCode);
		}
	}
	if (desc.AS.IsValid())
	{
		stream.stream2.AS = { desc.AS.ByteCode.data(), desc.AS.ByteCode.size() };
		if (cold.RootSignature == nullptr)
		{
			cold.RootSignature = CreateRootSignature(desc.AS.ByteCode);
		}
	}
	if (desc.VS.IsValid())
	{
		stream.stream1.VS = { desc.VS.ByteCode.data(), desc.VS.ByteCode.size() };
		if (cold.RootSignature == nullptr)
		{
			cold.RootSignature = CreateRootSignature(desc.VS.ByteCode);
		}
	}
	if (desc.HS.IsValid())
	{
		stream.stream1.HS = { desc.HS.ByteCode.data(), desc.HS.ByteCode.size() };
		if (cold.RootSignature == nullptr)
		{
			cold.RootSignature = CreateRootSignature(desc.HS.ByteCode);
		}
	}
	if (desc.DS.IsValid())
	{
		stream.stream1.DS = { desc.DS.ByteCode.data(), desc.DS.ByteCode.size() };
		if (cold.RootSignature == nullptr)
		{
			cold.RootSignature = CreateRootSignature(desc.DS.ByteCode);
		}
	}
	if (desc.GS.IsValid())
	{
		stream.stream1.GS = { desc.GS.ByteCode.data(), desc.GS.ByteCode.size() };
		if (cold.RootSignature == nullptr)
		{
			cold.RootSignature = CreateRootSignature(desc.GS.ByteCode);
		}
	}
	if (desc.PS.IsValid())
	{
		stream.stream1.PS = { desc.PS.ByteCode.data(), desc.PS.ByteCode.size() };
		if (cold.RootSignature == nullptr)
		{
			cold.RootSignature = CreateRootSignature(desc.PS.ByteCode);
		}
	}

	if (cold.RootSignature == nullptr)
	{
		cold.RootSignature = m_emptyRootSignature;
	}

	stream.stream1.ROOTSIG = cold.RootSignature.Get();

	TranslateBlendState(desc.BlendState, stream.stream1.BD);
	TranslateDepthStencilState(desc.DepthStencilState, stream.stream1.DSS);
	TranslateRasterState(desc.RasterState, stream.stream1.RS);

	D3D12_INPUT_LAYOUT_DESC il = {};

	std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
	if (!desc.VertexBufferBindings.IsEmpty())
	{
		il.NumElements = (uint32_t)desc.VertexBufferBindings.size();
		elements.resize(il.NumElements);
		for (uint32_t i = 0; i < il.NumElements; ++i)
		{
			auto& element = desc.VertexBufferBindings[i];
			D3D12_INPUT_ELEMENT_DESC& dx12Desc = elements[i];

			dx12Desc.SemanticName = element.SemanticName;
			dx12Desc.SemanticIndex = 0;


			const DxgiFormatMapping& formatMapping = GetDxgiFormatMapping(element.Format);
			dx12Desc.Format = formatMapping.SrvFormat;
			dx12Desc.InputSlot = element.InputSlot;
			dx12Desc.AlignedByteOffset = element.AlignedByteOffset;
			if (dx12Desc.AlignedByteOffset == VertexBufferBinding::sAppendAlignedElement)
			{
				dx12Desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			}

			if (element.InputSlotClass == InputClassification::PerInstanceData)
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

	}
	il.pInputElementDescs = elements.data();
	stream.stream1.IL = il;

	stream.stream1.SampleMask = desc.SampleMask;

	hot.Topology = ConvertPrimitiveTopology(desc.PrimType, desc.PatchControlPoints);
	switch (desc.PrimType)
	{
	case PrimitiveType::PointList:
		stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case PrimitiveType::LineList:
		stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case PrimitiveType::TriangleList:
	case PrimitiveType::TriangleStrip:
		stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case PrimitiveType::PatchList:
		stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		break;
	}
	stream.stream1.STRIP = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	DXGI_FORMAT DSFormat = GetDxgiFormatMapping(desc.RenderPassInfo.DsFormat).RtvFormat;
	D3D12_RT_FORMAT_ARRAY formats = {};
	formats.NumRenderTargets = (UINT)desc.RenderPassInfo.RTFormats.size();
	for (uint32_t i = 0; i < formats.NumRenderTargets; ++i)
	{
		formats.RTFormats[i] = GetDxgiFormatMapping(desc.RenderPassInfo.RTFormats[i]).RtvFormat;
	}

	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = desc.RenderPassInfo.SampleCount;
	sampleDesc.Quality = 0;

	stream.stream1.DSFormat = DSFormat;
	stream.stream1.Formats = formats;
	stream.stream1.SampleDesc = sampleDesc;

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
	streamDesc.pPipelineStateSubobjectStream = &stream;
	streamDesc.SizeInBytes = sizeof(stream.stream1);

	if (EnumHasAnyFlags(m_capabilities, DeviceCapability::MeshShading))
	{
		streamDesc.SizeInBytes += sizeof(stream.stream2);
	}

	HRESULT hr = m_d3d12Device2->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&hot.D3D12PipelineState));
	if (FAILED(hr))
	{
		PollDebugMessages();
		return false;
	}

	return true;
}


void GfxDeviceDx12::Present(SwapChain_Hot& hot, SwapChain_Cold const& cold)
{
	// -- Mark Queues for completion ---
	{
		const size_t backBufferIndex = cold.SwapChain4->GetCurrentBackBufferIndex();
		// -- Mark queues for Compleition ---
		for (size_t q = 0; q < (size_t)CommandQueueType::Count; q++)
		{
			D3D12CommandQueue& queue = m_commandQueues[q];
			queue.Queue->Signal(m_frameFences[backBufferIndex][q].Get(), 1);
		}

		// m_tempPageAllocator.EndFrame(GetGfxQueue().Queue.Get());
	}

	// -- Present the back buffer ---
	{

		UINT presentFlags = 0;
		if (!cold.VSync)
		{
			presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
		}

		HRESULT hr = cold.SwapChain4->Present((UINT)cold.VSync, presentFlags);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			assert(false);
		}
	}

	// -- wait for fence to finish
	{
		const size_t backBufferIndex = cold.SwapChain4->GetCurrentBackBufferIndex();

		m_frameCount++;

		// Sync The queeus
		for (size_t q = 0; q < (size_t)CommandQueueType::Count; q++)
		{
			ID3D12Fence* fence = m_frameFences[backBufferIndex][q].Get();
			const size_t completedValue = fence->GetCompletedValue();

			if (m_frameCount >= kBufferCount && completedValue < 1)
			{
				ThrowIfFailed(
					fence->SetEventOnCompletion(1, NULL));
			}
			// Reset fence;
			fence->Signal(0);
		}

		hot.CurrentRtv			= cold.ViewAllocation.GetCpuHandle((uint32_t)backBufferIndex);
		hot.CurrentBackBuffer	= cold.BackBuffers[backBufferIndex].Get();
	}
}


void GfxDeviceDx12::PollDebugMessages()
{
	::PollDebugMessages(GetD3D12Device());
}


void GfxDeviceDx12::Initialize()
{
	PHX_CORE_INFO("Initialize DirectX 12 Graphics Device");

	InitializeD3D12Context();

#if ENABLE_PIX_CAPUTRE
	m_pixCaptureModule = PIXLoadLatestWinPixGpuCapturerLibrary();
#endif 

	// Create Queues
	m_commandQueues[CommandQueueType::Graphics].Initialize(GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_commandQueues[CommandQueueType::Compute].Initialize(GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_commandQueues[CommandQueueType::Copy].Initialize(GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_COPY);

	// Create Descriptor Heaps
	m_cpuDescriptorHeaps[DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
		m_d3d12Device2,
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_cpuDescriptorHeaps[DescriptorHeapTypes::Sampler].Initialize(
		m_d3d12Device2,
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	m_cpuDescriptorHeaps[DescriptorHeapTypes::RTV].Initialize(
		m_d3d12Device2,
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	m_cpuDescriptorHeaps[DescriptorHeapTypes::DSV].Initialize(
		m_d3d12Device2,
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


	m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
		m_d3d12Device2,
		NUM_BINDLESS_RESOURCES,
		TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE - NUM_BINDLESS_RESOURCES,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);


	m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].Initialize(
		m_d3d12Device2,
		10,
		100,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	for (auto& frameFences : m_frameFences)
	{
		for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
		{
			ThrowIfFailed(
				GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFences[q])));
		}
	}

	D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
	allocatorDesc.pDevice = GetD3D12Device();
	allocatorDesc.pAdapter = m_gpuAdapter.NativeAdapter.Get();
	//allocatorDesc.PreferredBlockSize = 256 * 1024 * 1024;
	//allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
	allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS)(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);

	ThrowIfFailed(
		D3D12MA::CreateAllocator(&allocatorDesc, &m_d3d12MemAllocator));

	m_bindlessDescritorTable.Initialize(
		m_gpuDescriptorHeaps[0].Allocate(NUM_BINDLESS_RESOURCES));

	// m_tempPageAllocator.Initialize(256_MiB);

}

void GfxDeviceDx12::InitializeResourcePools()
{
}

void GfxDeviceDx12::FinalizeResourcePools()
{
}

void GfxDeviceDx12::InitializeD3D12Context()
{
	{
		uint32_t useDebugLayers = 0;
#if _DEBUG
		// Default to true for debug builds
		useDebugLayers = 1;
#endif
		CommandLineArgs::GetInteger(L"debug", useDebugLayers);
		m_debugLayersEnabled = (bool)useDebugLayers;
	}

	m_factory = CreateDXGIFactory6(m_debugLayersEnabled);
	FindAdapter(m_factory, m_gpuAdapter);

	if (!m_gpuAdapter.NativeAdapter)
	{
		// LOG_CORE_ERROR("Unable to create D3D12 RHI On current platform.");
	}

	ThrowIfFailed(
		D3D12CreateDevice(
			m_gpuAdapter.NativeAdapter.Get(),
			D3D_FEATURE_LEVEL_11_1,
			IID_PPV_ARGS(&m_d3d12Device)));

	m_d3d12Device->SetName(L"D3D12GfxDevice::RootDevice");
	Microsoft::WRL::ComPtr<IUnknown> renderdoc;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, RenderdocUUID, &renderdoc)))
	{
		m_isUnderGraphicsDebugger |= !!renderdoc;
	}

	Microsoft::WRL::ComPtr<IUnknown> pix;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, PixUUID, &pix)))
	{
		m_isUnderGraphicsDebugger |= !!pix;
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};
	bool hasOptions = SUCCEEDED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

	if (hasOptions)
	{
		if (featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
		{
			m_capabilities |= DeviceCapability::RT_VT_ArrayIndex_Without_GS;
		}
	}

	// TODO: Move to acability array
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
	bool hasOptions5 = SUCCEEDED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

	if (SUCCEEDED(m_d3d12Device.As(&m_d3d12Device5)) && hasOptions5)
	{
		if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
		{
			m_capabilities |= DeviceCapability::RayTracing;
		}
		if (featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0)
		{
			m_capabilities |= DeviceCapability::RenderPass;
		}
		if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
		{
			m_capabilities |= DeviceCapability::RayQuery;
		}
	}


	D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
	bool hasOptions6 = SUCCEEDED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

	if (hasOptions6)
	{
		if (featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
		{
			m_capabilities |= DeviceCapability::VariableRateShading;
		}
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
	bool hasOptions7 = SUCCEEDED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

	if (SUCCEEDED(m_d3d12Device.As(&m_d3d12Device2)) && hasOptions7)
	{
		if (featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
		{
			m_capabilities |= DeviceCapability::MeshShading;
		}
		m_capabilities |= DeviceCapability::CreateNoteZeroed;
	}

	m_featureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_d3d12Device2->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &m_featureDataRootSignature, sizeof(m_featureDataRootSignature))))
	{
		m_featureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Check shader model support
	m_featureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
	m_minShaderModel = ShaderModel::SM_6_6;
	if (FAILED(m_d3d12Device2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &m_featureDataShaderModel, sizeof(m_featureDataShaderModel))))
	{
		m_featureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
		m_minShaderModel = ShaderModel::SM_6_5;
	}

	if (m_debugLayersEnabled)
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(m_d3d12Device->QueryInterface<ID3D12InfoQueue>(&infoQueue)))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);

			D3D12_MESSAGE_SEVERITY severities[] =
			{
				D3D12_MESSAGE_SEVERITY_ERROR,
				D3D12_MESSAGE_SEVERITY_WARNING,
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


#if false

void GfxDeviceDx12::CreateSwapChain(SwapChainDesc const& desc, HWND hwnd)
{
	HRESULT hr;

	m_swapChain.ClearColour = desc.OptmizedClearValue;
	m_swapChain.VSync = desc.VSync;
	m_swapChain.Fullscreen = desc.Fullscreen;
	m_swapChain.EnableHDR = desc.EnableHDR;

	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	const auto& formatMapping = GetDxgiFormatMapping(desc.Format);
	if (m_swapChain.SwapChain == nullptr)
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
		swapChainDesc.BufferCount = kBufferCount;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags = swapChainFlags;

		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
		fullscreenDesc.Windowed = !desc.Fullscreen;

		hr = m_factory->CreateSwapChainForHwnd(
			GetGfxQueue().Queue.Get(),
			hwnd,
			&swapChainDesc,
			&fullscreenDesc,
			nullptr,
			m_swapChain.SwapChain.GetAddressOf()
		);

		if (FAILED(hr))
		{
			throw std::exception();
		}

		hr = m_swapChain.SwapChain.As(&m_swapChain.SwapChain4);
		if (FAILED(hr))
		{
			throw std::exception();
		}
	}
	else
	{
		// Resize swapchain:
		WaitForIdle();

		// Delete back buffers
		m_swapChain.Rtv.Free();
		for (auto& backBuffer : m_swapChain.BackBuffers)
		{
			backBuffer.Reset();
		}

		hr = m_swapChain.SwapChain->ResizeBuffers(
			kBufferCount,
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

	m_swapChain.Rtv = m_cpuDescriptorHeaps[DescriptorHeapTypes::RTV].Allocate(kBufferCount);
	for (UINT i = 0; i < kBufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource>& backBuffer = m_swapChain.BackBuffers[i];
		ThrowIfFailed(
			m_swapChain.SwapChain4->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		char allocatorName[32];
		sprintf_s(allocatorName, "Back Buffer %iu", i);

		GetD3D12Device()->CreateRenderTargetView(backBuffer.Get(), nullptr, m_swapChain.Rtv.GetCpuHandle(i));
	}
}

#endif