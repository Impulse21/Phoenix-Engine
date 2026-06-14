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

#include <optional>

namespace phx::rhi::vulkan
{
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

    struct VulkanContext
    {
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

        VmaAllocator    vma_allocator       = VK_NULL_HANDLE;

        // -- Resource Bools ---
        struct ResourcePools
        {
            phx::SmallObjectPool<Viewport, vulkan::ViewportImpl> viewports;
        } resource_pools;
    };

    inline static VulkanContext g_context;
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