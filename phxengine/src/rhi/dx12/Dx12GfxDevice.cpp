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
bool GfxDeviceDx12::CreateSwapChain(SwapChainDescriptor const& desc, SwapChain& swapChain)
{
	HRESULT hr;

	swapChain.ClearColour = desc.OptmizedClearValue;
	swapChain.VSync = desc.VSync;
	swapChain.Fullscreen = desc.Fullscreen;
	swapChain.EnableHDR = desc.EnableHDR;

	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	const auto& formatMapping = GetDxgiFormatMapping(desc.Format);
	if (swapChain.SwapChain == nullptr)
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
			swapChain.SwapChain.GetAddressOf()
		);

		if (FAILED(hr))
		{
			throw std::exception();
		}

		hr = swapChain.SwapChain.As(&swapChain.SwapChain4);
		if (FAILED(hr))
		{
			throw std::exception();
		}
	}
	else
	{
		// Resize swapchain:
		WaitForIdle();

		swapChain.CurrentIndex = 0;

		// Delete back buffers
		swapChain.ViewAllocation.Free();
		for (auto& backBuffer : swapChain.BackBuffers)
		{
			backBuffer.Reset();
		}

		hr = swapChain.SwapChain->ResizeBuffers(
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

	swapChain.ViewAllocation = m_cpuDescriptorHeaps[DescriptorHeapTypes::RTV].Allocate(kBufferCount);
	for (UINT i = 0; i < kBufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource>& backBuffer = swapChain.BackBuffers[i];
		ThrowIfFailed(
			pipeline.SwapChain4->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		char allocatorName[32];
		sprintf_s(allocatorName, "Back Buffer %iu", i);

		GetD3D12Device()->CreateRenderTargetView(backBuffer.Get(), nullptr, pipeline.ViewAllocation.GetCpuHandle(i));
	}

	swapChain.CurrentIndex = (uint32_t)swapChain.SwapChain4->GetCurrentBackBufferIndex();

	return true;
}

bool GfxDeviceDx12::CreatePipeline(PipelineStateDescriptor const& desc, PipelineState& pipeline)
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
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = CreateRootSignature(desc.MS.ByteCode);
		}
	}
	if (desc.AS.IsValid())
	{
		stream.stream2.AS = { desc.AS.ByteCode.data(), desc.AS.ByteCode.size() };
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = CreateRootSignature(desc.AS.ByteCode);
		}
	}
	if (desc.VS.IsValid())
	{
		stream.stream1.VS = { desc.VS.ByteCode.data(), desc.VS.ByteCode.size() };
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = CreateRootSignature(desc.VS.ByteCode);
		}
	}
	if (desc.HS.IsValid())
	{
		stream.stream1.HS = { desc.HS.ByteCode.data(), desc.HS.ByteCode.size() };
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = CreateRootSignature(desc.HS.ByteCode);
		}
	}
	if (desc.DS.IsValid())
	{
		stream.stream1.DS = { desc.DS.ByteCode.data(), desc.DS.ByteCode.size() };
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = CreateRootSignature(desc.DS.ByteCode);
		}
	}
	if (desc.GS.IsValid())
	{
		stream.stream1.GS = { desc.GS.ByteCode.data(), desc.GS.ByteCode.size() };
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = CreateRootSignature(desc.GS.ByteCode);
		}
	}
	if (desc.PS.IsValid())
	{
		stream.stream1.PS = { desc.PS.ByteCode.data(), desc.PS.ByteCode.size() };
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = CreateRootSignature(desc.PS.ByteCode);
		}
	}

	if (pipeline.RootSignature == nullptr)
	{
		pipeline.RootSignature = m_emptyRootSignature;
	}

	stream.stream1.ROOTSIG = pipeline.RootSignature.Get();

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

	pipeline.Topology = ConvertPrimitiveTopology(desc.PrimType, desc.PatchControlPoints);
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

	HRESULT hr = m_d3d12Device2->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipeline.D3D12PipelineState));
	if (FAILED(hr))
	{
		PollDebugMessages();
		return false;
	}

	return true;
}

bool GfxDeviceDx12::CreateTexture(TextureDescriptor const& desc, Texture& texture, MemInfo* initData)
{
	D3D12_CLEAR_VALUE d3d12OptimizedClearValue = {};
	d3d12OptimizedClearValue.Color[0] = desc.ClearValue.Colour.R;
	d3d12OptimizedClearValue.Color[1] = desc.ClearValue.Colour.G;
	d3d12OptimizedClearValue.Color[2] = desc.ClearValue.Colour.B;
	d3d12OptimizedClearValue.Color[3] = desc.ClearValue.Colour.A;
	d3d12OptimizedClearValue.DepthStencil.Depth = desc.ClearValue.DepthStencil.Depth;
	d3d12OptimizedClearValue.DepthStencil.Stencil = desc.ClearValue.DepthStencil.Stencil;

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

	switch (desc.Type)
	{
	case TextureType::Texture1D:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex1D(EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::TypelessFormatCasting) ? dxgiFormatMapping.SrvFormat : dxgiFormatMapping.RtvFormat,
				desc.Width,
				desc.ArraySize,
				desc.MipLevels,
				resourceFlags);
		break;
	}
	case TextureType::Texture2D:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex2D(EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::TypelessFormatCasting) ? dxgiFormatMapping.SrvFormat : dxgiFormatMapping.RtvFormat,
				desc.Width,
				desc.Height,
				desc.ArraySize,
				desc.MipLevels,
				1,
				0,
				resourceFlags);
		break;
	}
	case TextureType::Texture3D:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex3D(
				EnumHasAnyFlags(desc.MiscFlags, ResourceMiscFlags::TypelessFormatCasting) ? dxgiFormatMapping.SrvFormat : dxgiFormatMapping.RtvFormat,
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

	texture.MipLevels = desc.MipLevels;
	texture.ArraySize = desc.ArraySize;

	// TODO: Support aliasing
	ThrowIfFailed(
		m_d3d12MemAllocator->CreateResource(
			&allocationDesc,
			&resourceDesc,
			ConvertResourceStates(desc.InitialState),
			useClearValue ? &d3d12OptimizedClearValue : nullptr,
			&texture.Allocation,
			IID_PPV_ARGS(&texture.Resource)));


	std::wstring debugName;
	StringConvert(desc.DebugName, debugName);
	texture.Resource->SetName(debugName.c_str());

	if (initData)
	{
		UINT64 totalSize = 0;
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(desc.ArraySize * std::max((uint16_t)1u, (desc.MipLevels)));
		std::vector<UINT> numRows(footprints.size());
		std::vector<UINT64> rowSizesInBytes((UINT64)footprints.size());

		m_d3d12Device2->GetCopyableFootprints(
			&resourceDesc,
			0,
			(UINT)footprints.size(),
			0,
			footprints.data(),
			numRows.data(),
			rowSizesInBytes.data(),
			&totalSize);


		CopyCtxManager::Ctx ctx = m_copyCtxManager.Begin(totalSize);

		D3D12Buffer* uploadBufferImpl = m_resourceRegistry.Buffers.Get(ctx.UploadBuffer);

		for (size_t i = 0; i < footprints.size(); ++i)
		{
			D3D12_SUBRESOURCE_DATA data = {};
			data.RowPitch = initialData[i].rowPitch;
			data.SlicePitch = initialData[i].slicePitch;
			data.pData = initialData[i].pData;

			if (rowSizesInBytes[i] > (SIZE_T)-1)
				continue;

			D3D12_MEMCPY_DEST DestData = {};
			DestData.pData = (void*)((UINT64)ctx.MappedData + footprints[i].Offset);
			DestData.RowPitch = (SIZE_T)footprints[i].Footprint.RowPitch;
			DestData.SlicePitch = (SIZE_T)footprints[i].Footprint.RowPitch * (SIZE_T)numRows[i];
			MemcpySubresource(&DestData, &data, (SIZE_T)rowSizesInBytes[i], numRows[i], footprints[i].Footprint.Depth);

			if (ctx.IsValid())
			{
				CD3DX12_TEXTURE_COPY_LOCATION Dst(textureImpl.D3D12Resource.Get(), UINT(i));
				CD3DX12_TEXTURE_COPY_LOCATION Src(uploadBufferImpl->D3D12Resource.Get(), footprints[i]);
				ctx.CommandList->CopyTextureRegion(
					&Dst,
					0,
					0,
					0,
					&Src,
					nullptr
				);
			}
		}

		if (ctx.IsValid())
		{
			m_copyCtxManager.Submit(ctx);
		}

	}


	if ((desc.BindingFlags & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		CreateSubresource(texture, desc, SubresouceType::SRV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
	{
		CreateSubresource(texture, desc, SubresouceType::RTV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
	{
		CreateSubresource(texture, desc, SubresouceType::DSV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		CreateSubresource(texture, desc, SubresouceType::UAV, 0, ~0u, 0, ~0u);
	}

	return true;
}

void GfxDeviceDx12::Present(SwapChain& swapchain)
{
	// -- Mark Queues for completion ---
	{
		const size_t backBufferIndex = swapchain.SwapChain4->GetCurrentBackBufferIndex();
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
		if (!swapchain.VSync)
		{
			presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
		}

		HRESULT hr = swapchain.SwapChain4->Present((UINT)swapchain.VSync, presentFlags);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			assert(false);
		}
	}

	// -- wait for fence to finish
	{
		const size_t backBufferIndex = swapchain.SwapChain4->GetCurrentBackBufferIndex();

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

		swapchain.CurrentIndex = backBufferIndex;
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

int GfxDeviceDx12::CreateSubresource(Texture& hot, TextureDescriptor const& desc, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	switch (subresourceType)
	{
	case SubresouceType::SRV:
		return CreateShaderResourceView(hot, desc, firstSlice, sliceCount, firstMip, mipCount);
	case SubresouceType::UAV:
		return CreateUnorderedAccessView(hot, desc, firstSlice, sliceCount, firstMip, mipCount);
	case SubresouceType::RTV:
		return CreateRenderTargetView(hot, desc, firstSlice, sliceCount, firstMip, mipCount);
	case SubresouceType::DSV:
		return CreateDepthStencilView(hot, desc, firstSlice, sliceCount, firstMip, mipCount);
	default:
		throw std::runtime_error("Unknown sub resource type");
	}
}


#if false
int GfxDeviceDx12::CreateShaderResourceView(BufferHandle buffer, BufferDesc const& desc, size_t offset, size_t size)
{
	D3D12Buffer* bufferImpl = m_resourceRegistry.Buffers.Get(buffer);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

	if (desc.Format == Format::UNKNOWN)
	{
		if (EnumHasAnyFlags(desc.MiscFlags, BufferMiscFlags::Raw))
		{
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			srvDesc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
			srvDesc.Buffer.NumElements = (UINT)std::min(size, desc.SizeInBytes - offset) / sizeof(uint32_t);
		}
		else if (EnumHasAnyFlags(desc.MiscFlags, BufferMiscFlags::Structured))
		{
			// This is a Structured Buffer
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.FirstElement = (UINT)offset / desc.Stride;
			srvDesc.Buffer.NumElements = (UINT)std::min(size, desc.SizeInBytes - offset) / desc.Stride;
			srvDesc.Buffer.StructureByteStride = desc.Stride;
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
			.Allocation = D3D12GpuDevice::GetResourceCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.SRVDesc = srvDesc,
	};

	GetD3D12Device2()->CreateShaderResourceView(
		bufferImpl->D3D12Resource.Get(),
		&srvDesc,
		view.Allocation.GetCpuHandle());

	const bool isBindless = true;
	if (isBindless)
	{
		// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
		view.BindlessIndex = m_bindlessDescritorTable.Allocate();
		if (view.BindlessIndex != cInvalidDescriptorIndex)
		{
			GetD3D12Device2()->CopyDescriptorsSimple(
				1,
				m_bindlessDescritorTable.GetCpuHandle(view.BindlessIndex),
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

int GfxDeviceDx12::CreateUnorderedAccessView(BufferHandle buffer, BufferDesc const& desc, size_t offset, size_t size)
{
	D3D12Buffer* bufferImpl = m_resourceRegistry.Buffers.Get(buffer);;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

	const bool hasCounter = (desc.MiscFlags & BufferMiscFlags::HasCounter) == BufferMiscFlags::HasCounter;
	if (desc.Format == Format::UNKNOWN)
	{
		if (EnumHasAnyFlags(desc.MiscFlags, BufferMiscFlags::Raw))
		{
			uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
			uavDesc.Buffer.FirstElement = (UINT)offset / sizeof(uint32_t);
			uavDesc.Buffer.NumElements = (UINT)std::min(size, desc.SizeInBytes - offset) / sizeof(uint32_t);
		}
		else if (EnumHasAnyFlags(desc.MiscFlags, BufferMiscFlags::Structured))
		{
			// This is a Structured Buffer
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.Buffer.FirstElement = (UINT)offset / desc.Stride;
			uavDesc.Buffer.NumElements = (UINT)std::min(size, desc.SizeInBytes - offset) / desc.Stride;
			uavDesc.Buffer.StructureByteStride = desc.Stride;

			if (hasCounter)
			{
				// uavDesc.Buffer.CounterOffsetInBytes = desc.UavCounterOffsetInBytes;
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
			.Allocation = GetResourceCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.UAVDesc = uavDesc,
	};

	ID3D12Resource* counterResource = nullptr;
	if (hasCounter)
	{
		if (desc.UavCounterBuffer.IsValid())
		{
			D3D12Buffer* counterBuffer = m_resourceRegistry.Buffers.Get(desc.UavCounterBuffer);
			counterResource = counterBuffer->D3D12Resource.Get();

		}
		else
		{
			counterResource = bufferImpl->D3D12Resource.Get();
		}
	}

	GetD3D12Device2()->CreateUnorderedAccessView(
		bufferImpl->D3D12Resource.Get(),
		counterResource,
		&uavDesc,
		view.Allocation.GetCpuHandle());

	const bool isBindless = true;
	if (isBindless)
	{
		// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
		view.BindlessIndex = m_bindlessDescritorTable.Allocate();
		if (view.BindlessIndex != cInvalidDescriptorIndex)
		{
			GetD3D12Device2()->CopyDescriptorsSimple(
				1,
				m_bindlessDescritorTable.GetCpuHandle(view.BindlessIndex),
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
#endif

int GfxDeviceDx12::CreateShaderResourceView(Texture& hot, TextureDescriptor const& desc, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = dxgiFormatMapping.SrvFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	uint32_t planeSlice = (srvDesc.Format == DXGI_FORMAT_X24_TYPELESS_G8_UINT) ? 1 : 0;

	switch (desc.Type)
	{
	case TextureType::Texture1D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		rtvDesc.Texture1D.MipSlice = firstMip;
		break;
	case TextureType::Texture1DArray:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		rtvDesc.Texture1DArray.FirstArraySlice = firstSlice;
		rtvDesc.Texture1DArray.ArraySize = sliceCount;
		rtvDesc.Texture1DArray.MipSlice = firstMip;
		break;
	case TextureType::Texture2D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = firstMip;
		break;
	case TextureType::Texture2DArray:
	case TextureType::TextureCube:
	case TextureType::TextureCubeArray:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.ArraySize = sliceCount;
		rtvDesc.Texture2DArray.FirstArraySlice = firstSlice;
		rtvDesc.Texture2DArray.MipSlice = firstMip;
		break;
	case TextureType::Texture2DMS:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		break;
	case TextureType::Texture2DMSArray:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		rtvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
		rtvDesc.Texture2DMSArray.ArraySize = sliceCount;
		break;
	case TextureType::Texture3D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		rtvDesc.Texture3D.FirstWSlice = 0;
		rtvDesc.Texture3D.WSize = desc.ArraySize;
		rtvDesc.Texture3D.MipSlice = firstMip;
		break;
	case TextureType::Unknown:
	default:
		throw std::runtime_error("Unsupported Enum");
	}

	DescriptorView view = {
			.Allocation = GetResourceCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.SRVDesc = srvDesc,
			.FirstMip = firstMip,
			.MipCount = mipCount,
			.FirstSlice = firstSlice,
			.SliceCount = sliceCount
	};

	GetD3D12Device2()->CreateShaderResourceView(
		pipeline.Resource.Get(),
		&srvDesc,
		view.Allocation.GetCpuHandle());

	if (true)
	{
		// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
		view.BindlessIndex = m_bindlessDescritorTable.Allocate();
		if (view.BindlessIndex != cInvalidDescriptorIndex)
		{
			GetD3D12Device2()->CopyDescriptorsSimple(
				1,
				m_bindlessDescritorTable.GetCpuHandle(view.BindlessIndex),
				view.Allocation.GetCpuHandle(),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}
	if (hot->Srv.Allocation.IsNull())
	{
		textureImpl->Srv = view;
		return -1;
	}

	textureImpl->SrvSubresourcesAlloc.push_back(view);
	return textureImpl->SrvSubresourcesAlloc.size() - 1;
}

int GfxDeviceDx12::CreateRenderTargetView(Texture& hot, TextureDescriptor const& desc, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = dxgiFormatMapping.RtvFormat;

	switch (desc.Type)
	{
	case TextureType::Texture1D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		rtvDesc.Texture1D.MipSlice = firstMip;
		break;
	case TextureType::Texture1DArray:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		rtvDesc.Texture1DArray.FirstArraySlice = firstSlice;
		rtvDesc.Texture1DArray.ArraySize = sliceCount;
		rtvDesc.Texture1DArray.MipSlice = firstMip;
		break;
	case TextureType::Texture2D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = firstMip;
		break;
	case TextureType::Texture2DArray:
	case TextureType::TextureCube:
	case TextureType::TextureCubeArray:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.ArraySize = sliceCount;
		rtvDesc.Texture2DArray.FirstArraySlice = firstSlice;
		rtvDesc.Texture2DArray.MipSlice = firstMip;
		break;
	case TextureType::Texture2DMS:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		break;
	case TextureType::Texture2DMSArray:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		rtvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
		rtvDesc.Texture2DMSArray.ArraySize = sliceCount;
		break;
	case TextureType::Texture3D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		rtvDesc.Texture3D.FirstWSlice = 0;
		rtvDesc.Texture3D.WSize = desc.ArraySize;
		rtvDesc.Texture3D.MipSlice = firstMip;
		break;
	case TextureType::Unknown:
	default:
		throw std::runtime_error("Unsupported Enum");
	}

	DescriptorView view = {
			.Allocation = GetRtvCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			.RTVDesc = rtvDesc,
			.FirstMip = firstMip,
			.MipCount = mipCount,
			.FirstSlice = firstSlice,
			.SliceCount = sliceCount
	};

	GetD3D12Device2()->CreateRenderTargetView(
		pipeline.Resource.Get(),
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

int GfxDeviceDx12::CreateDepthStencilView(Texture& hot, TextureDescriptor const& desc, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = dxgiFormatMapping.RtvFormat;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	switch (desc.Type)
	{
	case TextureType::Texture1D:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
		dsvDesc.Texture1D.MipSlice = firstMip;
		break;
	case TextureType::Texture1DArray:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		dsvDesc.Texture1DArray.FirstArraySlice = firstSlice;
		dsvDesc.Texture1DArray.ArraySize = sliceCount;
		dsvDesc.Texture1DArray.MipSlice = firstMip;
		break;
	case TextureType::Texture2D:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = firstMip;
		break;
	case TextureType::Texture2DArray:
	case TextureType::TextureCube:
	case TextureType::TextureCubeArray:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.ArraySize = sliceCount;
		dsvDesc.Texture2DArray.FirstArraySlice = firstSlice;
		dsvDesc.Texture2DArray.MipSlice = firstMip;
		break;
	case TextureType::Texture2DMS:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
		break;
	case TextureType::Texture2DMSArray:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		dsvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
		dsvDesc.Texture2DMSArray.ArraySize = sliceCount;
		break;
	case TextureType::Texture3D:
	{
		throw std::runtime_error("Unsupported Dimension");
	}
	case TextureType::Unknown:
	default:
		throw std::runtime_error("Unsupported Enum");
	}

	DescriptorView view = {
			.Allocation = GetDsvCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			.DSVDesc = dsvDesc,
			.FirstMip = firstMip,
			.MipCount = mipCount,
			.FirstSlice = firstSlice,
			.SliceCount = sliceCount
	};

	GetD3D12Device2()->CreateDepthStencilView(
		pipeline.Resource.Get(),
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

int GfxDeviceDx12::CreateUnorderedAccessView(Texture& hot, TextureDescriptor const& desc, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
	D3D12Texture* textureImpl = m_resourceRegistry.Textures.Get(texture);

	auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = dxgiFormatMapping.SrvFormat;

	switch (desc.Type)
	{
	case TextureType::Texture1D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		uavDesc.Texture1D.MipSlice = firstMip;
		break;
	case TextureType::Texture1DArray:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		uavDesc.Texture1DArray.FirstArraySlice = firstSlice;
		uavDesc.Texture1DArray.ArraySize = sliceCount;
		uavDesc.Texture1DArray.MipSlice = firstMip;
		break;
	case TextureType::Texture2D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = firstMip;
		break;
	case TextureType::Texture2DArray:
	case TextureType::TextureCube:
	case TextureType::TextureCubeArray:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.FirstArraySlice = firstSlice;
		uavDesc.Texture2DArray.ArraySize = sliceCount;
		uavDesc.Texture2DArray.MipSlice = firstMip;
		break;
	case TextureType::Texture3D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.FirstWSlice = 0;
		uavDesc.Texture3D.WSize = desc.Depth;
		uavDesc.Texture3D.MipSlice = firstMip;
		break;
	case TextureType::Texture2DMS:
	case TextureType::Texture2DMSArray:
	{
		throw std::runtime_error("Unsupported Dimension");
	}
	case TextureType::Unknown:
	default:
		throw std::runtime_error("Unsupported Enum");
	}

	DescriptorView view = {
			.Allocation = GetResourceCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.UAVDesc = uavDesc,
			.FirstMip = firstMip,
			.MipCount = mipCount,
			.FirstSlice = firstSlice,
			.SliceCount = sliceCount
	};

	GetD3D12Device2()->CreateUnorderedAccessView(
		pipeline.Resource.Get(),
		nullptr,
		&uavDesc,
		view.Allocation.GetCpuHandle());

	if (true)
	{
		// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
		view.BindlessIndex = m_bindlessDescritorTable.Allocate();
		if (view.BindlessIndex != cInvalidDescriptorIndex)
		{
			GetD3D12Device2()->CopyDescriptorsSimple(
				1,
				m_bindlessDescritorTable.GetCpuHandle(view.BindlessIndex),
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