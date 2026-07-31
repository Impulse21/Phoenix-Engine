#pragma once


// ============================================================
//  PHX Engine — Vulkan Initialisation Reference
//  Target API: Vulkan 1.4
// ============================================================
//
//  INSTANCE EXTENSIONS
//  ─────────────────────────────────────────────────────────────────────────
//  Extension                       Debug   Notes
//  ─────────────────────────────────────────────────────────────────────────
//  VK_KHR_surface                  always  Base surface abstraction
//  VK_KHR_win32_surface            always  Windows platform surface
//  VK_KHR_wayland_surface          always  Linux/Wayland platform surface
//  VK_EXT_debug_utils              debug   Message callback, object naming
//
//  INSTANCE LAYERS
//  ─────────────────────────────────────────────────────────────────────────
//  VK_LAYER_KHRONOS_validation     debug   CPU-side validation
//
//  INSTANCE VALIDATION FEATURES  (chained into VkInstanceCreateInfo.pNext)
//  ─────────────────────────────────────────────────────────────────────────
//  GPU_ASSISTED                    debug   Catches invalid BDAs, OOB descriptors
//  BEST_PRACTICES                  debug   Driver/perf warnings
//  SYNCHRONIZATION_VALIDATION      debug   Missing barriers, layout transitions
//
//  DEVICE EXTENSIONS — REQUIRED
//  ─────────────────────────────────────────────────────────────────────────
//  VK_KHR_swapchain                        Presentation; deliberately not core
//  VK_EXT_descriptor_buffer                Descriptor heap-style binding model
//
//  DEVICE EXTENSIONS — OPTIONAL  (queried, enabled if present, cap flag set)
//  ─────────────────────────────────────────────────────────────────────────
//  VK_EXT_mesh_shader                      Meshlet pipeline (task + mesh stages)
//  VK_KHR_ray_query                        Inline RT in any shader stage
//  VK_KHR_acceleration_structure           BLAS/TLAS; required by ray query
//  VK_KHR_deferred_host_operations         Async PSO/BLAS build; required by accel
//  VK_EXT_shader_object                    Pipeline-free shader binding
//  VK_EXT_calibrated_timestamps            Correlate GPU timestamps to CPU clock
//  VK_EXT_multi_draw                       Batch vkCmdDraw calls, cheaper than indirect for small counts
//
//  CORE FEATURES — REQUIRED  (enforced in CheckRequiredFeatures / ScoreDevice)
//  ─────────────────────────────────────────────────────────────────────────
//  Vulkan 1.1
//    multiview                             Shadow cascades / stereo in one pass
//  Vulkan 1.2
//    bufferDeviceAddress                   BDA; also required by VMA for device address allocs
//    descriptorIndexing                    Bindless descriptor arrays
//    drawIndirectCount                     GPU-driven draw count from buffer (vkCmdDrawIndexedIndirectCount)
//    timelineSemaphore                     Frame graph sync without fence/semaphore pairs
//    hostQueryReset                        Reset timestamp pools from CPU
//    samplerMirrorClampToEdge              Texture quality; widely supported
//  Vulkan 1.3
//    dynamicRendering                      Renderpass-free rendering (vkCmdBeginRendering)
//    synchronization2                      vkCmdPipelineBarrier2; cleaner barrier API
//    maintenance4                          Relaxed buffer/image requirements, spec constant workgroup size
//  Vulkan 1.4
//    maintenance6                          Null descriptor sets in bind calls; reduces validation noise
//  EXT
//    descriptorBuffer                      Must match VK_EXT_descriptor_buffer extension above
//
//  CORE FEATURES — OPTIONAL  (scored during device selection, enabled if present)
//  ─────────────────────────────────────────────────────────────────────────
//  VK_EXT_mesh_shader    meshShader        Meshlet geometry stage
//                        taskShader        Amplification / per-meshlet culling
//  VK_KHR_ray_query      rayQuery          Inline shadow rays, AO in compute
//
//  VMA ALLOCATOR FLAGS
//  ─────────────────────────────────────────────────────────────────────────
//  BUFFER_DEVICE_ADDRESS           Required for any allocation queried via vkGetBufferDeviceAddress.
//                                  Sets VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT on underlying VkDeviceMemory.
//                                  Without this, BDA returns garbage.
//  EXT_MEMORY_BUDGET               Queries real VRAM budget from driver for better eviction decisions.
//                                  VK_EXT_memory_budget is core in Vulkan 1.4.
//
//  DEVICE SCORING  (PhxVkSelectPhysicalDevice)
//  ─────────────────────────────────────────────────────────────────────────
//  Disqualifiers (score == 0):
//    Missing required extension or feature (see REQUIRED above)
//    No graphics+compute queue family
//  Discrete GPU                    +3000   Dominant weight; discrete always wins
//  Integrated GPU                  +1000   Usable fallback
//  Dedicated async compute queue   +500    COMPUTE family without GRAPHICS
//  Dedicated transfer queue        +500    TRANSFER family without GRAPHICS or COMPUTE (DMA engine)
//  maxImageDimension2D             +N      Tiebreak between equal devices
//  Mesh shaders present            +200    Optional feature bonus
//  Ray query present               +200    Optional feature bonus
//
// ============================================================
 


#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Pool.h>

#if defined(PHX_PLATFORM_WINDOWS)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(PHX_PLATFORM_LINUX)
#define VK_USE_PLATFORM_WAYLAND_KHR
#else
#error "Unsupported Platform"
#endif

#include <volk.h>
#include <vk_mem_alloc.h>

#include "RHIVulkanResources.h"
#include "DeferredCallbackQueue.h"

#include <optional>

namespace phx::rhi::vulkan
{
    // TODO: Drive via CVar
    constexpr u32 k_max_raw_per_frame = 32;

    struct QueueFamilyIndices
    {
        std::optional<u32> graphics_family = std::nullopt;
        std::optional<u32> async_compute_family = std::nullopt;
        std::optional<u32> async_transfer_family = std::nullopt;

        bool IsComplete() const
        {
            return graphics_family.has_value();
        }
        bool HasAsyncCompute() const
        {
            return async_compute_family.has_value() &&
                async_compute_family.value() != graphics_family.value();
        }
        bool HasAsyncTransfer() const
        {
            return async_transfer_family.has_value() &&
                async_transfer_family.value() != graphics_family.value() &&
                async_transfer_family.value() != async_compute_family.value();
        }
    };

    struct FrameContext
    {
        VkCommandPool       vk_cmd_buffer_pool = VK_NULL_HANDLE;
        VkCommandBuffer     vk_cmd_buffers[k_max_raw_per_frame];
        CommandBufferHandle begin_frame_cmd_handle;
        CommandBufferHandle end_frame_cmd_handle;
        u32                 cmd_in_use = 0;
    };

    struct VulkanContext
    {
        constexpr static uint32_t kMaxInflightFrames = 2;

        u64                         frame_number = 0;
        
        VkInstance                  vk_instance         = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT    debug_messenger     = VK_NULL_HANDLE;

        VkPhysicalDevice            vk_physical_device              = VK_NULL_HANDLE;
        QueueFamilyIndices          queue_family_indices            = {};
        VkPhysicalDeviceProperties  vk_physical_device_properties   = {};

        RhiCapabilities capabilities        = {};
        VkDevice        vk_device           = VK_NULL_HANDLE;

        VkQueue         vk_gfx_queue        = VK_NULL_HANDLE;
        VkQueue         vk_present_queue    = VK_NULL_HANDLE;
        VkQueue         vk_compute_queue    = VK_NULL_HANDLE;
        VkQueue         vk_transfer_queue   = VK_NULL_HANDLE;

        IHeapAllocator* allocator           = nullptr;
        VmaAllocator    vma_allocator       = VK_NULL_HANDLE;

        DeferredCallbackQueue<kMaxInflightFrames> deferred_callback_queue;
     
        VkSemaphore     vk_timeline_sem = VK_NULL_HANDLE;
        u64             frame_wait_values[kMaxInflightFrames];
   
        // -- Frame info ---
        FrameContext frame_ctx[kMaxInflightFrames];
        u32 max_cmd_buffers_per_thread = 0;

        // -- Resource Pools ---
        phx::SmallObjectPool<Viewport, vulkan::ViewportImpl, 4>             pool_viewports;
        phx::SmallObjectPool<CommandBuffer, vulkan::CommandBufferImpl, 16>  pool_cmd_buffer;
        phx::Pool<GpuBuffer, VulkanBuffer>                                  pool_gpu_buffers;
        phx::Pool<Texture, VulkanTexture>                                   pool_textures;
        phx::Pool<PipelineState, VulkanPipelineState>                       pool_pipeline_states;
        phx::Pool<Sampler, VulkanSampler>                                   pool_samplers;
        phx::Pool<ShaderModule, VulkanShaderModule>                         pool_shader_modules;

        // -- Helpers ---
        u64 GetCurrentFrame() const { return (frame_number + 1) % kMaxInflightFrames; }
        FrameContext&  GetCurrentFrameCtx() { return frame_ctx[GetCurrentFrame()]; }
        const FrameContext&  GetCurrentFrameCtx() const { return frame_ctx[GetCurrentFrame()]; }

    };

    inline VulkanContext g_context;
    
}

#define vulkan_check(call)                                          \
    do {                                                            \
        VkResult _vk_res = (call);                                  \
        if (_vk_res != VK_SUCCESS)                                  \
        {                                                           \
            PHX_LOG_ERROR(Log::Channels::RHI,                       \
                "Vulkan call failed: {} = {}",                      \
                #call, static_cast<int>(_vk_res));                  \
            PHX_ASSERT(false);                                      \
            std::abort();                                           \
        }                                                           \
    } while(0)

namespace phx::rhi::vulkan
{
    constexpr VkFormat gVulkanFormatMapping[] = {
        VK_FORMAT_UNDEFINED,                 // UNKNOWN
        VK_FORMAT_R8_UINT,                   // R8_UINT
        VK_FORMAT_R8_SINT,                   // R8_SINT
        VK_FORMAT_R8_UNORM,                  // R8_UNORM
        VK_FORMAT_R8_SNORM,                  // R8_SNORM
        VK_FORMAT_R8G8_UINT,                 // RG8_UINT
        VK_FORMAT_R8G8_SINT,                 // RG8_SINT
        VK_FORMAT_R8G8_UNORM,                // RG8_UNORM
        VK_FORMAT_R8G8_SNORM,                // RG8_SNORM
        VK_FORMAT_R16_UINT,                  // R16_UINT
        VK_FORMAT_R16_SINT,                  // R16_SINT
        VK_FORMAT_R16_UNORM,                 // R16_UNORM
        VK_FORMAT_R16_SNORM,                 // R16_SNORM
        VK_FORMAT_R16_SFLOAT,                // R16_FLOAT
        VK_FORMAT_B4G4R4A4_UNORM_PACK16,     // BGRA4_UNORM
        VK_FORMAT_B5G6R5_UNORM_PACK16,       // B5G6R5_UNORM
        VK_FORMAT_B5G5R5A1_UNORM_PACK16,     // B5G5R5A1_UNORM
        VK_FORMAT_R8G8B8A8_UINT,             // RGBA8_UINT
        VK_FORMAT_R8G8B8A8_SINT,             // RGBA8_SINT
        VK_FORMAT_R8G8B8A8_UNORM,            // RGBA8_UNORM
        VK_FORMAT_R8G8B8A8_SNORM,            // RGBA8_SNORM
        VK_FORMAT_B8G8R8A8_UNORM,            // BGRA8_UNORM
        VK_FORMAT_R8G8B8A8_SRGB,             // SRGBA8_UNORM
        VK_FORMAT_B8G8R8A8_SRGB,             // SBGRA8_UNORM
        VK_FORMAT_A2R10G10B10_UNORM_PACK32,  // R10G10B10A2_UNORM
        VK_FORMAT_B10G11R11_UFLOAT_PACK32,   // R11G11B10_FLOAT
        VK_FORMAT_R16G16_UINT,               // RG16_UINT
        VK_FORMAT_R16G16_SINT,               // RG16_SINT
        VK_FORMAT_R16G16_UNORM,              // RG16_UNORM
        VK_FORMAT_R16G16_SNORM,              // RG16_SNORM
        VK_FORMAT_R16G16_SFLOAT,             // RG16_FLOAT
        VK_FORMAT_R32_UINT,                  // R32_UINT
        VK_FORMAT_R32_SINT,                  // R32_SINT
        VK_FORMAT_R32_SFLOAT,                // R32_FLOAT
        VK_FORMAT_R16G16B16A16_UINT,         // RGBA16_UINT
        VK_FORMAT_R16G16B16A16_SINT,         // RGBA16_SINT
        VK_FORMAT_R16G16B16A16_SFLOAT,       // RGBA16_FLOAT
        VK_FORMAT_R16G16B16A16_UNORM,        // RGBA16_UNORM
        VK_FORMAT_R16G16B16A16_SNORM,        // RGBA16_SNORM
        VK_FORMAT_R32G32_UINT,               // RG32_UINT
        VK_FORMAT_R32G32_SINT,               // RG32_SINT
        VK_FORMAT_R32G32_SFLOAT,             // RG32_FLOAT
        VK_FORMAT_R32G32B32_UINT,            // RGB32_UINT
        VK_FORMAT_R32G32B32_SINT,            // RGB32_SINT
        VK_FORMAT_R32G32B32_SFLOAT,          // RGB32_FLOAT
        VK_FORMAT_R32G32B32A32_UINT,         // RGBA32_UINT
        VK_FORMAT_R32G32B32A32_SINT,         // RGBA32_SINT
        VK_FORMAT_R32G32B32A32_SFLOAT,       // RGBA32_FLOAT

        VK_FORMAT_D16_UNORM,            // D16
        VK_FORMAT_D24_UNORM_S8_UINT,    // D24S8
        VK_FORMAT_X8_D24_UNORM_PACK32,  // X24G8_UINT
        VK_FORMAT_D32_SFLOAT,           // D32
        VK_FORMAT_D32_SFLOAT_S8_UINT,   // D32S8
        VK_FORMAT_X8_D24_UNORM_PACK32,  // X32G8_UINT

        VK_FORMAT_BC1_RGB_UNORM_BLOCK,  // BC1_UNORM
        VK_FORMAT_BC1_RGB_SRGB_BLOCK,   // BC1_UNORM_SRGB
        VK_FORMAT_BC2_UNORM_BLOCK,      // BC2_UNORM
        VK_FORMAT_BC2_SRGB_BLOCK,       // BC2_UNORM_SRGB
        VK_FORMAT_BC3_UNORM_BLOCK,      // BC3_UNORM
        VK_FORMAT_BC3_SRGB_BLOCK,       // BC3_UNORM_SRGB
        VK_FORMAT_BC4_UNORM_BLOCK,      // BC4_UNORM
        VK_FORMAT_BC4_SNORM_BLOCK,      // BC4_SNORM
        VK_FORMAT_BC5_UNORM_BLOCK,      // BC5_UNORM
        VK_FORMAT_BC5_SNORM_BLOCK,      // BC5_SNORM
        VK_FORMAT_BC6H_UFLOAT_BLOCK,    // BC6H_UFLOAT
        VK_FORMAT_BC6H_SFLOAT_BLOCK,    // BC6H_SFLOAT
        VK_FORMAT_BC7_UNORM_BLOCK,      // BC7_UNORM
        VK_FORMAT_BC7_SRGB_BLOCK,       // BC7_UNORM_SRGB
    };

    static_assert(sizeof(gVulkanFormatMapping) / sizeof(VkFormat) ==
                (int)rhi::Format::COUNT);

    // static assert
    constexpr VkFormat FormatToVkFormat(rhi::Format format)
    {
        return gVulkanFormatMapping[(int)format];
    }

    constexpr std::array<VkShaderStageFlagBits, (size_t)ShaderStage::Count>
        kShaderStageToVk = {
            VK_SHADER_STAGE_MESH_BIT_EXT,  // MS
            VK_SHADER_STAGE_TASK_BIT_EXT,  // AS (Amplification == Task in Vulkan)
            VK_SHADER_STAGE_VERTEX_BIT,    // VS
            VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,  // HS (Hull == Tess Control)
            VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,  // DS (Domain == Tess
                                                        // Eval)
            VK_SHADER_STAGE_GEOMETRY_BIT,                 // GS
            VK_SHADER_STAGE_FRAGMENT_BIT,                 // PS (Pixel == Fragment)
            VK_SHADER_STAGE_COMPUTE_BIT,                  // CS
            VK_SHADER_STAGE_ALL                           // LIB (Generic Library)
    };

    // Helper for cleaner casting
    inline VkShaderStageFlagBits ShaderStageToVulkanShaderStage(ShaderStage stage)
    {
        return kShaderStageToVk[static_cast<uint8_t>(stage)];
    }

    constexpr VkComponentSwizzle ComponentSwizzleMap[] = {
        VK_COMPONENT_SWIZZLE_R,     // ComponentSwizzle::R
        VK_COMPONENT_SWIZZLE_G,     // ComponentSwizzle::G
        VK_COMPONENT_SWIZZLE_B,     // ComponentSwizzle::B
        VK_COMPONENT_SWIZZLE_A,     // ComponentSwizzle::A
        VK_COMPONENT_SWIZZLE_ZERO,  // ComponentSwizzle::Zero
        VK_COMPONENT_SWIZZLE_ONE,   // ComponentSwizzle::One
    };

    constexpr VkComponentMapping ConvertSwizzle(Swizzle value)
    {
        VkComponentMapping mapping = {};
        mapping.r = ComponentSwizzleMap[(size_t)value.r];
        mapping.g = ComponentSwizzleMap[(size_t)value.g];
        mapping.b = ComponentSwizzleMap[(size_t)value.b];
        mapping.a = ComponentSwizzleMap[(size_t)value.a];
        return mapping;
    }

    constexpr VkBlendFactor ConvertBlendValue(BlendFactor value)
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

    constexpr VkColorComponentFlags ToVkColorComponentFlags(ColorMask mask)
    {
        // Cast to integer to allow bitwise operations on enum class
        const uint8_t m = static_cast<uint8_t>(mask);

        VkColorComponentFlags flags = 0;

        if (m & static_cast<uint8_t>(rhi::ColorMask::Red))
            flags |= VK_COLOR_COMPONENT_R_BIT;
        if (m & static_cast<uint8_t>(rhi::ColorMask::Green))
            flags |= VK_COLOR_COMPONENT_G_BIT;
        if (m & static_cast<uint8_t>(rhi::ColorMask::Blue))
            flags |= VK_COLOR_COMPONENT_B_BIT;
        if (m & static_cast<uint8_t>(rhi::ColorMask::Alpha))
            flags |= VK_COLOR_COMPONENT_A_BIT;

        return flags;
    }
    constexpr VkBlendOp ConvertBlendOp(EBlendOp value)
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

    constexpr VkStencilOp ConvertStencilOp(StencilOp value)
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

    constexpr VkCompareOp ConvertComparisonFunc(ComparisonFunc value)
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

    constexpr bool IsFormatStencilSupport(Format format)
    {
        switch (format)
        {
            case Format::D32S8:
            case Format::D24S8:
                return true;
            default:
                return false;
        }
    }
    constexpr VkImageLayout ResourceStateToImageLayout(ResourceStates value)
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
                // Workaround for handling multiple queues with textures in
                // different layouts
                return VK_IMAGE_LAYOUT_GENERAL;
            case ResourceStates::ShadingRateSurface:
                return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
            case ResourceStates::Present:
                return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            default:
                // Combination of state flags will default to general
                return VK_IMAGE_LAYOUT_GENERAL;
        }
    }
    constexpr VkAccessFlags2 ResourceStateToAccessFlags2(ResourceStates value)
    {
        VkAccessFlags2 flags = VK_ACCESS_2_NONE;

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
            flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        }
        if (EnumHasAnyFlags(value, ResourceStates::ShadingRateSurface))
        {
            flags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
        }
        // Assuming the following states are added to map to the original
        // VIDEO_DECODE_DST and VIDEO_DECODE_SRC
        if (EnumHasAnyFlags(value, ResourceStates::AccelStructWrite))
        {
            flags |= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        }
        if (EnumHasAnyFlags(value, ResourceStates::AccelStructBuildInput))
        {
            flags |= VK_ACCESS_2_SHADER_READ_BIT;
        }

        return flags;
    }

    constexpr VkPipelineStageFlags ResourceStateToPipelineStage(
        rhi::ResourceStates states)
    {
        VkPipelineStageFlags vk_stages = VK_PIPELINE_STAGE_2_NONE;

        // --- Helper Masks ---
        // Note: Assumes you have KHR extensions enabled in your Vulkan context
        constexpr VkPipelineStageFlags ALL_SHADER_STAGES =
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
            VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
            VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
            VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

        constexpr VkPipelineStageFlags NON_PIXEL_SHADER_STAGES =
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
            VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
            VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
            VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

        if (EnumHasAnyFlags(states, ResourceStates::ConstantBuffer))
        {
            vk_stages |= ALL_SHADER_STAGES;
        }

        if (EnumHasAnyFlags(states, ResourceStates::VertexBuffer) ||
            EnumHasAnyFlags(states, ResourceStates::IndexGpuBuffer))
        {
            vk_stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        }

        if (EnumHasAnyFlags(states, ResourceStates::IndirectArgument))
        {
            vk_stages |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
        }

        if (EnumHasAnyFlags(states, ResourceStates::ShaderResource))
        {
            vk_stages |= ALL_SHADER_STAGES;
        }

        if (EnumHasAnyFlags(states, ResourceStates::UnorderedAccess))
        {
            vk_stages |= ALL_SHADER_STAGES;
        }

        if (EnumHasAnyFlags(states, ResourceStates::RenderTarget))
        {
            vk_stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }

        if (EnumHasAnyFlags(states, ResourceStates::DepthWrite))
        {
            vk_stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        }

        if (EnumHasAnyFlags(states, ResourceStates::DepthRead))
        {
            // Can be read via tests OR as a shader resource
            vk_stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        if (EnumHasAnyFlags(states, ResourceStates::StreamOut))
        {
            vk_stages |=
                VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT;  // Requires
                                                            // VK_EXT_transform_feedback
        }

        if (EnumHasAnyFlags(states, ResourceStates::CopyDest) ||
            EnumHasAnyFlags(states, ResourceStates::CopySource) ||
            EnumHasAnyFlags(states, ResourceStates::ResolveDest) ||
            EnumHasAnyFlags(states, ResourceStates::ResolveSource))
        {
            vk_stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
        }

        if (EnumHasAnyFlags(states, ResourceStates::Present))
        {
            // Sync against the final operations before present
            vk_stages |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }

        if (EnumHasAnyFlags(states, ResourceStates::AccelStructRead))
        {
            // AS can be read by RT, Compute, or even Vertex shaders
            vk_stages |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR |
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
        }

        if (EnumHasAnyFlags(states, ResourceStates::AccelStructWrite) ||
            EnumHasAnyFlags(states, ResourceStates::AccelStructBuildInput) ||
            EnumHasAnyFlags(states, ResourceStates::AccelStructBuildBlas))
        {
            vk_stages |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        }

        if (EnumHasAnyFlags(states, ResourceStates::ShadingRateSurface))
        {
            vk_stages |=
                VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;  // Requires
                                                                            // VK_KHR_fragment_shading_rate
        }

        if (EnumHasAnyFlags(states, ResourceStates::GenericRead))
        {
            // This is a broad "read" state
            vk_stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
                        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                        VK_PIPELINE_STAGE_TRANSFER_BIT | ALL_SHADER_STAGES;
        }

        if (EnumHasAnyFlags(states, ResourceStates::ShaderResourceNonPixel))
        {
            vk_stages |= NON_PIXEL_SHADER_STAGES;
        }

        // If no state was specified, default to a safe value
        if (vk_stages == 0)
        {
            vk_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }

        return vk_stages;
    }

    constexpr VkPrimitiveTopology ToVkPrimtivieTopology(rhi::PrimitiveType type)
    {
        switch (type)
        {
            case PrimitiveType::PointList:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case PrimitiveType::LineList:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveType::LineStrip:
                return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case PrimitiveType::TriangleList:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PrimitiveType::PatchList:
                return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
            case PrimitiveType::TriangleStrip:
            default:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        }
    }

#if false
constexpr VkPipelineBindPoint ToVkPipelineBindPoint(PipelineType type)
{
    switch (type)
    {
        case PipelineType::Graphics:
            return VK_PIPELINE_BIND_POINT_GRAPHICS;

        case PipelineType::Compute:
            return VK_PIPELINE_BIND_POINT_COMPUTE;

        case PipelineType::RayTracing:
            return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;

        default:
            // This should never happen.
            // Return Graphics as a safe fallback to prevent uninitialized
            // usage, but in debug builds this should ideally assert.
            return VK_PIPELINE_BIND_POINT_GRAPHICS;
    }
}
#endif

    inline VkCullModeFlags ToVkCullMode(rhi::RasterCullMode mode)
    {
        switch (mode)
        {
            case rhi::RasterCullMode::None:
                return VK_CULL_MODE_NONE;
            case rhi::RasterCullMode::Front:
                return VK_CULL_MODE_FRONT_BIT;
            case rhi::RasterCullMode::Back:
                return VK_CULL_MODE_BACK_BIT;
            default:
                return VK_CULL_MODE_FRONT_AND_BACK;
        }
    }

    inline VkFrontFace ToVkFrontFace(FrontFace face)
    {
        return (face == FrontFace::CounterClockwise)
                ? VK_FRONT_FACE_COUNTER_CLOCKWISE
                : VK_FRONT_FACE_CLOCKWISE;
    }

    inline VkImageViewType ToVkImageViewType(TextureType type)
    {
        switch (type)
        {
            case TextureType::Texture1D:
                return VK_IMAGE_VIEW_TYPE_1D;
            case TextureType::Texture1DArray:
                return VK_IMAGE_VIEW_TYPE_1D_ARRAY;

            case TextureType::Texture2D:
                return VK_IMAGE_VIEW_TYPE_2D;
            case TextureType::Texture2DArray:
                return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

            case TextureType::TextureCube:
                return VK_IMAGE_VIEW_TYPE_CUBE;
            case TextureType::TextureCubeArray:
                return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

            case TextureType::Texture2DMS:
                return VK_IMAGE_VIEW_TYPE_2D;
            case TextureType::Texture2DMSArray:
                return VK_IMAGE_VIEW_TYPE_2D_ARRAY;

            case TextureType::Texture3D:
                return VK_IMAGE_VIEW_TYPE_3D;

            case TextureType::Unknown:
            default:
                return VK_IMAGE_VIEW_TYPE_2D;
        }
    }

    constexpr VkImageAspectFlags GetAspectFlags(VkFormat format)
    {
        switch (format)
        {
                // Depth + Stencil
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D16_UNORM_S8_UINT:
                return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

                // Depth Only
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_X8_D24_UNORM_PACK32:
                return VK_IMAGE_ASPECT_DEPTH_BIT;

                // Stencil Only (Rare)
            case VK_FORMAT_S8_UINT:
                return VK_IMAGE_ASPECT_STENCIL_BIT;

                // Everything else is Color
            default:
                return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }
}  // namespace phx::rhi::vulkan