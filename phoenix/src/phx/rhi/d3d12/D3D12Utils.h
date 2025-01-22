#pragma once

#include "phx/core/Log.h"
#include "phx/core/EnumUtils.h"
#include "phx/rhi/RHITypes.h"
#include "D3D12Core.h"

namespace phx::rhi::d3d12
{
	inline D3D12_RESOURCE_STATES ConvertResourceStates(ResourceStates stateBits)
	{
		if (stateBits == ResourceStates::Common)
			return D3D12_RESOURCE_STATE_COMMON;

		D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON; // also 0

		if (EnumHasAnyFlags(stateBits, ResourceStates::ConstantBuffer)) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (EnumHasAnyFlags(stateBits, ResourceStates::VertexBuffer)) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (EnumHasAnyFlags(stateBits, ResourceStates::IndexGpuBuffer)) result |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if (EnumHasAnyFlags(stateBits, ResourceStates::IndirectArgument)) result |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		if (EnumHasAnyFlags(stateBits, ResourceStates::ShaderResource)) result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::UnorderedAccess)) result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if (EnumHasAnyFlags(stateBits, ResourceStates::RenderTarget)) result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		if (EnumHasAnyFlags(stateBits, ResourceStates::DepthWrite)) result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::DepthRead)) result |= D3D12_RESOURCE_STATE_DEPTH_READ;
		if (EnumHasAnyFlags(stateBits, ResourceStates::StreamOut)) result |= D3D12_RESOURCE_STATE_STREAM_OUT;
		if (EnumHasAnyFlags(stateBits, ResourceStates::CopyDest)) result |= D3D12_RESOURCE_STATE_COPY_DEST;
		if (EnumHasAnyFlags(stateBits, ResourceStates::CopySource)) result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::ResolveDest)) result |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
		if (EnumHasAnyFlags(stateBits, ResourceStates::ResolveSource)) result |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::Present)) result |= D3D12_RESOURCE_STATE_PRESENT;
		if (EnumHasAnyFlags(stateBits, ResourceStates::AccelStructRead)) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::AccelStructWrite)) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::AccelStructBuildInput)) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::AccelStructBuildBlas)) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::ShadingRateSurface)) result |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
		if (EnumHasAnyFlags(stateBits, ResourceStates::GenericRead)) result |= D3D12_RESOURCE_STATE_GENERIC_READ;
		if (EnumHasAnyFlags(stateBits, ResourceStates::ShaderResourceNonPixel)) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

		return result;
	}

	inline D3D_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(PrimitiveType topology, uint32_t controlPoints)
	{
		switch (topology)
		{
		case PrimitiveType::PointList:
			return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case PrimitiveType::LineList:
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case PrimitiveType::LineStrip:
			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case PrimitiveType::TriangleList:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case PrimitiveType::TriangleStrip:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case PrimitiveType::PatchList:
			if (controlPoints == 0 || controlPoints > 32)
			{
				assert(false && "Invalid PatchList control points");
				return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
			}
			return D3D_PRIMITIVE_TOPOLOGY(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (controlPoints - 1));
		default:
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}

	inline D3D12_SHADER_VISIBILITY ConvertShaderStage(rhi::ShaderStage s)
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

	inline D3D12_BLEND ConvertBlendValue(BlendFactor value)
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

	inline D3D12_BLEND_OP ConvertBlendOp(EBlendOp value)
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

	inline D3D12_STENCIL_OP ConvertStencilOp(StencilOp value)
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

	inline D3D12_COMPARISON_FUNC ConvertComparisonFunc(ComparisonFunc value)
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

	inline D3D_PRIMITIVE_TOPOLOGY ConvertPrimitiveType(PrimitiveType pt, uint32_t controlPoints)
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

	inline D3D12_TEXTURE_ADDRESS_MODE ConvertSamplerAddressMode(SamplerAddressMode mode)
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

	inline UINT ConvertSamplerReductionType(SamplerReductionType reductionType)
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

	inline void PollDebugMessages(ID3D12Device* device)
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


	inline void TranslateBlendState(BlendRenderState const& inState, D3D12_BLEND_DESC& outState)
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

	inline void TranslateDepthStencilState(DepthStencilRenderState const& inState, D3D12_DEPTH_STENCIL_DESC& outState)
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

	inline void TranslateRasterState(RasterRenderState const& inState, D3D12_RASTERIZER_DESC& outState)
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
}
