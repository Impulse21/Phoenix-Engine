#pragma once

#include "EmberGfx/phxGfxDeviceResources.h"
#include <vulkan/vulkan.h>
namespace phx::gfx::platform
{
    constexpr VkFormat gVulkanFormatMapping[] = 
    {
       VK_FORMAT_UNDEFINED,                  // UNKNOWN
       VK_FORMAT_R8_UINT,                    // R8_UINT
       VK_FORMAT_R8_SINT,                    // R8_SINT
       VK_FORMAT_R8_UNORM,                   // R8_UNORM
       VK_FORMAT_R8_SNORM,                   // R8_SNORM
       VK_FORMAT_R8G8_UINT,                  // RG8_UINT
       VK_FORMAT_R8G8_SINT,                  // RG8_SINT
       VK_FORMAT_R8G8_UNORM,                 // RG8_UNORM
       VK_FORMAT_R8G8_SNORM,                 // RG8_SNORM
       VK_FORMAT_R16_UINT,                   // R16_UINT
       VK_FORMAT_R16_SINT,                   // R16_SINT
       VK_FORMAT_R16_UNORM,                  // R16_UNORM
       VK_FORMAT_R16_SNORM,                  // R16_SNORM
       VK_FORMAT_R16_SFLOAT,                 // R16_FLOAT
       VK_FORMAT_B4G4R4A4_UNORM_PACK16,      // BGRA4_UNORM
       VK_FORMAT_B5G6R5_UNORM_PACK16,        // B5G6R5_UNORM
       VK_FORMAT_B5G5R5A1_UNORM_PACK16,      // B5G5R5A1_UNORM
       VK_FORMAT_R8G8B8A8_UINT,              // RGBA8_UINT
       VK_FORMAT_R8G8B8A8_SINT,              // RGBA8_SINT
       VK_FORMAT_R8G8B8A8_UNORM,             // RGBA8_UNORM
       VK_FORMAT_R8G8B8A8_SNORM,             // RGBA8_SNORM
       VK_FORMAT_B8G8R8A8_UNORM,             // BGRA8_UNORM
       VK_FORMAT_R8G8B8A8_SRGB,              // SRGBA8_UNORM
       VK_FORMAT_B8G8R8A8_SRGB,              // SBGRA8_UNORM
       VK_FORMAT_A2R10G10B10_UNORM_PACK32,   // R10G10B10A2_UNORM
       VK_FORMAT_B10G11R11_UFLOAT_PACK32,    // R11G11B10_FLOAT
       VK_FORMAT_R16G16_UINT,                // RG16_UINT
       VK_FORMAT_R16G16_SINT,                // RG16_SINT
       VK_FORMAT_R16G16_UNORM,               // RG16_UNORM
       VK_FORMAT_R16G16_SNORM,               // RG16_SNORM
       VK_FORMAT_R16G16_SFLOAT,              // RG16_FLOAT
       VK_FORMAT_R32_UINT,                   // R32_UINT
       VK_FORMAT_R32_SINT,                   // R32_SINT
       VK_FORMAT_R32_SFLOAT,                 // R32_FLOAT
       VK_FORMAT_R16G16B16A16_UINT,          // RGBA16_UINT
       VK_FORMAT_R16G16B16A16_SINT,          // RGBA16_SINT
       VK_FORMAT_R16G16B16A16_SFLOAT,        // RGBA16_FLOAT
       VK_FORMAT_R16G16B16A16_UNORM,         // RGBA16_UNORM
       VK_FORMAT_R16G16B16A16_SNORM,         // RGBA16_SNORM
       VK_FORMAT_R32G32_UINT,                // RG32_UINT
       VK_FORMAT_R32G32_SINT,                // RG32_SINT
       VK_FORMAT_R32G32_SFLOAT,              // RG32_FLOAT
       VK_FORMAT_R32G32B32_UINT,             // RGB32_UINT
       VK_FORMAT_R32G32B32_SINT,             // RGB32_SINT
       VK_FORMAT_R32G32B32_SFLOAT,           // RGB32_FLOAT
       VK_FORMAT_R32G32B32A32_UINT,          // RGBA32_UINT
       VK_FORMAT_R32G32B32A32_SINT,          // RGBA32_SINT
       VK_FORMAT_R32G32B32A32_SFLOAT,        // RGBA32_FLOAT

       VK_FORMAT_D16_UNORM,                  // D16
       VK_FORMAT_D24_UNORM_S8_UINT,          // D24S8
       VK_FORMAT_X8_D24_UNORM_PACK32,        // X24G8_UINT
       VK_FORMAT_D32_SFLOAT,                 // D32
       VK_FORMAT_D32_SFLOAT_S8_UINT,         // D32S8
       VK_FORMAT_X8_D24_UNORM_PACK32,        // X32G8_UINT

       VK_FORMAT_BC1_RGB_UNORM_BLOCK,        // BC1_UNORM
       VK_FORMAT_BC1_RGB_SRGB_BLOCK,         // BC1_UNORM_SRGB
       VK_FORMAT_BC2_UNORM_BLOCK,            // BC2_UNORM
       VK_FORMAT_BC2_SRGB_BLOCK,             // BC2_UNORM_SRGB
       VK_FORMAT_BC3_UNORM_BLOCK,            // BC3_UNORM
       VK_FORMAT_BC3_SRGB_BLOCK,             // BC3_UNORM_SRGB
       VK_FORMAT_BC4_UNORM_BLOCK,            // BC4_UNORM
       VK_FORMAT_BC4_SNORM_BLOCK,            // BC4_SNORM
       VK_FORMAT_BC5_UNORM_BLOCK,            // BC5_UNORM
       VK_FORMAT_BC5_SNORM_BLOCK,            // BC5_SNORM
       VK_FORMAT_BC6H_UFLOAT_BLOCK,          // BC6H_UFLOAT
       VK_FORMAT_BC6H_SFLOAT_BLOCK,          // BC6H_SFLOAT
       VK_FORMAT_BC7_UNORM_BLOCK,            // BC7_UNORM
       VK_FORMAT_BC7_SRGB_BLOCK,             // BC7_UNORM_SRGB
    };
    
    static_assert(sizeof(gVulkanFormatMapping) / sizeof(VkFormat) == (int)gfx::Format::COUNT);

    // static assert
    VkFormat FormatToVkFormat(gfx::Format format)
    {
        return gVulkanFormatMapping[(int)format];
    }

	constexpr VkComponentSwizzle ComponentSwizzleMap[] = 
	{
		VK_COMPONENT_SWIZZLE_R,        // ComponentSwizzle::R
		VK_COMPONENT_SWIZZLE_G,        // ComponentSwizzle::G
		VK_COMPONENT_SWIZZLE_B,        // ComponentSwizzle::B
		VK_COMPONENT_SWIZZLE_A,        // ComponentSwizzle::A
		VK_COMPONENT_SWIZZLE_ZERO,     // ComponentSwizzle::Zero
		VK_COMPONENT_SWIZZLE_ONE,      // ComponentSwizzle::One
	};

	constexpr VkComponentMapping ConvertSwizzle(Swizzle value)
	{
		VkComponentMapping mapping = {};
		mapping.r = ComponentSwizzleMap[(size_t)value.R];
		mapping.g = ComponentSwizzleMap[(size_t)value.G];
		mapping.b = ComponentSwizzleMap[(size_t)value.B];
		mapping.a = ComponentSwizzleMap[(size_t)value.A];
		return mapping;
	}

	VkBlendFactor ConvertBlendValue(BlendFactor value)
	{
		switch (value)
		{
		case BlendFactor::Zero:
			return VK_BLEND_FACTOR_ZERO;
		case BlendFactor::One:
			return VK_BLEND_FACTOR_ONE;
		case BlendFactor::SrcColor:
			return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendFactor::InvSrcColor:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendFactor::SrcAlpha:
			return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendFactor::InvSrcAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DstAlpha:
			return VK_BLEND_FACTOR_DST_ALPHA;
		case BlendFactor::InvDstAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case BlendFactor::DstColor:
			return VK_BLEND_FACTOR_DST_COLOR;
		case BlendFactor::InvDstColor:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendFactor::SrcAlphaSaturate:
			return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		case BlendFactor::ConstantColor:
			return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case BlendFactor::InvConstantColor:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case BlendFactor::Src1Color:
			return VK_BLEND_FACTOR_SRC1_COLOR;
		case BlendFactor::InvSrc1Color:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		case BlendFactor::Src1Alpha:
			return VK_BLEND_FACTOR_SRC1_ALPHA;
		case BlendFactor::InvSrc1Alpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		default:
			return VK_BLEND_FACTOR_ZERO;
		}
	}

	VkBlendOp ConvertBlendOp(EBlendOp value)
	{
		switch (value)
		{
		case EBlendOp::Add:
			return VK_BLEND_OP_ADD;
		case EBlendOp::Subrtact:
			return VK_BLEND_OP_SUBTRACT;
		case EBlendOp::ReverseSubtract:
			return VK_BLEND_OP_REVERSE_SUBTRACT;
		case EBlendOp::Min:
			return VK_BLEND_OP_MIN;
		case EBlendOp::Max:
			return VK_BLEND_OP_MAX;
		default:
			return VK_BLEND_OP_ADD;
		}
	}

	VkStencilOp ConvertStencilOp(StencilOp value)
	{
		switch (value)
		{
		case StencilOp::Keep:
			return VK_STENCIL_OP_KEEP;
		case StencilOp::Zero:
			return VK_STENCIL_OP_ZERO;
		case StencilOp::Replace:
			return VK_STENCIL_OP_REPLACE;
		case StencilOp::IncrementAndClamp:
			return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case StencilOp::DecrementAndClamp:
			return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case StencilOp::Invert:
			return VK_STENCIL_OP_INVERT;
		case StencilOp::IncrementAndWrap:
			return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case StencilOp::DecrementAndWrap:
			return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default:
			return VK_STENCIL_OP_KEEP;
		}
	}

	VkCompareOp ConvertComparisonFunc(ComparisonFunc value)
	{
		switch (value)
		{
		case ComparisonFunc::Never:
			return VK_COMPARE_OP_NEVER;
		case ComparisonFunc::Less:
			return VK_COMPARE_OP_LESS;
		case ComparisonFunc::Equal:
			return VK_COMPARE_OP_EQUAL;
		case ComparisonFunc::LessOrEqual:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case ComparisonFunc::Greater:
			return VK_COMPARE_OP_GREATER;
		case ComparisonFunc::NotEqual:
			return VK_COMPARE_OP_NOT_EQUAL;
		case ComparisonFunc::GreaterOrEqual:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case ComparisonFunc::Always:
			return VK_COMPARE_OP_ALWAYS;
		default:
			return VK_COMPARE_OP_NEVER;
		}
	}

	constexpr bool IsFormatDepthSupport(Format format)
	{
		switch (format)
		{
		case Format::D16:
		case Format::D32:
		case Format::D24S8:
			return true;
		default:
			return false;
		}
	}

	constexpr VkImageLayout ConvertImageLayout(ResourceStates value)
	{
		switch (value)
		{
		case ResourceStates::Unknown:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case ResourceStates::RenderTarget:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ResourceStates::DepthWrite:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ResourceStates::DepthRead:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case ResourceStates::ShaderResource:
		case ResourceStates::ShaderResourceNonPixel:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ResourceStates::UnorderedAccess:
			return VK_IMAGE_LAYOUT_GENERAL;
		case ResourceStates::CopySource:
		case ResourceStates::CopyDest:
			// Workaround for handling multiple queues with textures in different layouts
			return VK_IMAGE_LAYOUT_GENERAL;
		case ResourceStates::ShadingRateSurface:
			return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
		default:
			// Combination of state flags will default to general
			return VK_IMAGE_LAYOUT_GENERAL;
		}
	}
	constexpr VkAccessFlags2 _ParseResourceState(ResourceStates value)
	{
		VkAccessFlags2 flags = 0;

		if (EnumHasAnyFlags(value, ResourceStates::ShaderResource))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::ShaderResourceNonPixel))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::UnorderedAccess))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
			flags |= VK_ACCESS_2_SHADER_WRITE_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::CopySource))
		{
			flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::CopyDest))
		{
			flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::RenderTarget))
		{
			flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
			flags |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::DepthWrite))
		{
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::DepthRead))
		{
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::VertexBuffer))
		{
			flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::IndexGpuBuffer))
		{
			flags |= VK_ACCESS_2_INDEX_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::ConstantBuffer))
		{
			flags |= VK_ACCESS_2_UNIFORM_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::IndirectArgument))
		{
			flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::AccelStructRead))
		{
			flags |= VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT;
		}
		if (EnumHasAnyFlags(value, ResourceStates::ShadingRateSurface))
		{
			flags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
		}
		// Assuming the following states are added to map to the original VIDEO_DECODE_DST and VIDEO_DECODE_SRC
		if (EnumHasAnyFlags(value, ResourceStates::AccelStructWrite))
		{
			flags |= VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
		}
		if (EnumHasAnyFlags(value, ResourceStates::AccelStructBuildInput))
		{
			flags |= VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR;
		}

		return flags;
	}

}