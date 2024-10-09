#pragma once

#include "EmberGfx/phxGfxDeviceResources.h"
#include <vulkan/vulkan.h>
namespace phx::gfx::platform
{
    VkFormat gVulkanFormatMapping[] = 
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
}