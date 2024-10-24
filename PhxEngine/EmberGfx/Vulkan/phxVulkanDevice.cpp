#include "pch.h"

#include "phxMemory.h"
#include "phxVulkanCore.h"
#include "phxVulkanDevice.h"
#include "phxMath.h"


#define VOLK_IMPLEMENTATION
#include "volk/volk.h"

#include "spriv-reflect/spirv_reflect.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vma/vk_mem_alloc.h"

#include <vulkan/vulkan_win32.h>

#include <set>
#include <map>

#define LOG_DEVICE_EXTENSIONS false

#define LOG_AND_FINALIZE_POOL(x) if (!x.IsEmpty()) PHX_CORE_WARN("[Vulkan] - Pool '" #x "' still contains active handles"); x.Finalize();
using namespace phx;
using namespace phx::gfx;
using namespace phx::gfx::platform;


namespace phx::EngineCore
{
    extern HWND g_hWnd;
    extern HINSTANCE g_hInstance;
}

namespace
{
    const std::vector<const char*> kRequiredExtensions = 
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    // Optional extensions can also be done in the same way
    const std::vector<const char*> kOptionalExtensions =
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };

    const std::vector<const char*> kValidationLayerPriorityList[] =
    {
        // The preferred validation layer is "VK_LAYER_KHRONOS_validation"
        {"VK_LAYER_KHRONOS_validation"},

        // Otherwise we fallback to using the LunarG meta layer
        {"VK_LAYER_LUNARG_standard_validation"},

        // Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
        {
            "VK_LAYER_GOOGLE_threading",
            "VK_LAYER_LUNARG_parameter_validation",
            "VK_LAYER_LUNARG_object_tracker",
            "VK_LAYER_LUNARG_core_validation",
            "VK_LAYER_GOOGLE_unique_objects",
        },

        // Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
        {"VK_LAYER_LUNARG_core_validation"}
    };

    const std::vector<const char*> kRequiredDeviceExtensions = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) 
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }


    PFN_vkSetDebugUtilsObjectNameEXT    pfnSetDebugUtilsObjectNameEXT;
    PFN_vkCmdBeginDebugUtilsLabelEXT    pfnCmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT      pfnCmdEndDebugUtilsLabelEXT;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) 
{
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    {
        PHX_CORE_WARN("[Vulkan] {0}", pCallbackData->pMessage);
        break;
    }
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    {
        PHX_CORE_ERROR("[Vulkan] {0}", pCallbackData->pMessage);
        break;
    }
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    {
        PHX_CORE_INFO("[Vulkan] {0}", pCallbackData->pMessage);
        break;
    }
    };

    return VK_FALSE;
}

void phx::gfx::platform::VulkanGpuDevice::Initialize(SwapChainDesc const& swapChainDesc, bool enableValidationLayers, void* windowHandle)
{
    volkInitialize();

    m_enableValidationLayers = enableValidationLayers;
    m_extManager.Initialize();
    if (m_enableValidationLayers)
        m_layerManager.Initialize();

    CreateInstance();
    if (m_enableValidationLayers)
        SetupDebugMessenger();

    CreateSurface(windowHandle);
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapchain(swapChainDesc);
    CreateSwapChaimImageViews();

	VkPipelineCacheCreateInfo cacheCreateInfo = {};
	cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    
	vkCreatePipelineCache(m_vkDevice, &cacheCreateInfo, nullptr, &m_vkPipelineCache);

    // Dynamic PSO states:
    m_psoDynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT);
    m_psoDynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT);
    m_psoDynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
    m_psoDynamicStates.push_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
#if false
    if (CheckCapability(GraphicsDeviceCapability::DEPTH_BOUNDS_TEST))
    {
        m_psoDynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
    }
    if (CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING))
    {
        m_psoDynamicStates.push_back(VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR);
    }
#endif
    m_psoDynamicStates.push_back(VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE);

    m_dynamicStateInfo = {};
    m_dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_dynamicStateInfo.pDynamicStates = m_psoDynamicStates.data();
    m_dynamicStateInfo.dynamicStateCount = (uint32_t)m_psoDynamicStates.size();

    m_dynamicStateInfo_MeshShader = {};
    m_dynamicStateInfo_MeshShader.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_dynamicStateInfo_MeshShader.pDynamicStates = m_psoDynamicStates.data();
    m_dynamicStateInfo_MeshShader.dynamicStateCount = (uint32_t)m_psoDynamicStates.size() - 1; // don't include VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE for mesh shader

    InitializeDescriptorHeaps();
    CreateVma();
    CreateFrameResources();
    CreateDefaultResources();
    m_dynamicAllocator.Initialize(this, 256_MiB);
    m_copyCtxManager.Initialize(this);
}

void phx::gfx::platform::VulkanGpuDevice::Finalize()
{
    WaitForIdle();

    m_dynamicAllocator.Finalize();
    m_copyCtxManager.Finalize();

    RunGarbageCollection();
    DestoryFrameResources();
    DestoryDefaultResources();

    FinalizeResourcePools();

    DestroyDescriptorHeaps();

    for (auto& cmdCtx : m_commandCtxPool)
    {
        for (uint32_t buffer = 0; buffer < kBufferCount; ++buffer)
        {
            for (size_t queue = 0; queue < (size_t)CommandQueueType::Count; ++queue)
            {
                vkDestroyCommandPool(m_vkDevice, cmdCtx->CmdBufferPoolVk[buffer][queue], nullptr);
            }
        }
    }

    m_commandCtxPool.clear();

    vkDestroyPipelineCache(m_vkDevice, m_vkPipelineCache, nullptr); 
    CleanupSwapchain();
    vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);

    vmaDestroyAllocator(m_vmaAllocator);
    vmaDestroyAllocator(m_vmaAllocatorExternal);
    vkDestroyDevice(m_vkDevice, nullptr);

    if (m_enableValidationLayers) 
    {
        DestroyDebugUtilsMessengerEXT(m_vkInstance, m_debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
    vkDestroyInstance(m_vkInstance, nullptr);
}

void phx::gfx::platform::VulkanGpuDevice::RunGarbageCollection(uint64_t completedFrame)
{
    while (!m_deferredQueue.empty())
    {
        DeferredItem& DeferredItem = m_deferredQueue.front();
        if (DeferredItem.Frame + kBufferCount < completedFrame)
        {
            DeferredItem.DeferredFunc();
            m_deferredQueue.pop_front();
        }
        else
        {
            break;
        }
    }
}

ICommandCtx* phx::gfx::platform::VulkanGpuDevice::BeginCommandCtx(phx::gfx::CommandQueueType type)
{
    CommandCtx_Vulkan* retVal = nullptr; 
    uint32_t ctxCurrent = ~0u;
    {
        std::scoped_lock _(m_commandPoolLock);
        ctxCurrent = m_commandCtxCount++;
        if (ctxCurrent >= m_commandCtxPool.size())
        {
            m_commandCtxPool.push_back(std::make_unique<CommandCtx_Vulkan>());
        }
        retVal = m_commandCtxPool[ctxCurrent].get();
    }

    retVal->Reset(GetBufferIndex());
    retVal->Id = ctxCurrent;
    retVal->QueueType = type;
    retVal->GpuDevice = this;

    if (retVal->GetVkCommandBuffer() == VK_NULL_HANDLE)
    {
        for (uint32_t buffer = 0; buffer < kBufferCount; ++buffer)
        {
            VkCommandPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            switch (type)
            {
            case CommandQueueType::Graphics:
                poolInfo.queueFamilyIndex = m_queueFamilies.GraphicsFamily.value();
                break;
            case CommandQueueType::Compute:
                poolInfo.queueFamilyIndex = m_queueFamilies.ComputeFamily.value();
                break;
            case CommandQueueType::Copy:
                poolInfo.queueFamilyIndex = m_queueFamilies.TransferFamily.value();
                break;
            default:
                assert(0); // queue type not handled
                break;
            }
            poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

            VkResult res = vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &retVal->CmdBufferPoolVk[buffer][type]);
            assert(res == VK_SUCCESS);

            VkCommandBufferAllocateInfo commandBufferInfo = {};
            commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferInfo.commandBufferCount = 1;
            commandBufferInfo.commandPool = retVal->CmdBufferPoolVk[buffer][type];
            commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

            res = vkAllocateCommandBuffers(m_vkDevice, &commandBufferInfo, &retVal->CmdBufferVk[buffer][type]);
            assert(res == VK_SUCCESS);
        }
    }


    VkResult res = vkResetCommandPool(m_vkDevice, retVal->GetVkCommandPool(), 0);
    assert(res == VK_SUCCESS);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr; // Optional

    res = vkBeginCommandBuffer(retVal->GetVkCommandBuffer(), &beginInfo);

    if (type == CommandQueueType::Graphics)
    {
        vkCmdSetRasterizerDiscardEnable(retVal->GetVkCommandBuffer(), VK_FALSE);

        VkViewport vp = {};
        vp.width = 1;
        vp.height = 1;
        vp.maxDepth = 1;
        vkCmdSetViewportWithCount(retVal->GetVkCommandBuffer(), 1, &vp);

        VkRect2D scissor;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = 65535;
        scissor.extent.height = 65535;
        vkCmdSetScissorWithCount(retVal->GetVkCommandBuffer(), 1, &scissor);

        float blendConstants[] = { 1,1,1,1 };
        vkCmdSetBlendConstants(retVal->GetVkCommandBuffer(), blendConstants);

        vkCmdSetStencilReference(retVal->GetVkCommandBuffer(), VK_STENCIL_FRONT_AND_BACK, ~0u);

        if (m_features2.features.depthBounds == VK_TRUE)
        {
            vkCmdSetDepthBounds(retVal->GetVkCommandBuffer(), 0.0f, 1.0f);
        }

        //  Need to bind all dynamic states even if it's not being used.
        const VkDeviceSize zero = {};
        vkCmdBindVertexBuffers2(retVal->GetVkCommandBuffer(), 0, 1, &m_nullBuffer, &zero, &zero, &zero);
#if false

        if (CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING))
        {
            VkExtent2D fragmentSize = {};
            fragmentSize.width = 1;
            fragmentSize.height = 1;

            VkFragmentShadingRateCombinerOpKHR combiner[] = {
                VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
                VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR
            };

            vkCmdSetFragmentShadingRateKHR(
                retVal->GetVkCommandBuffer(),
                &fragmentSize,
                combiner
            );
        }
#endif
    }

    return retVal;
}

void phx::gfx::platform::VulkanGpuDevice::SubmitFrame()
{
    SubmitCommandCtx();
    Present();
    RunGarbageCollection();
}

void phx::gfx::platform::VulkanGpuDevice::WaitForIdle()
{
    VkResult res = vkDeviceWaitIdle(m_vkDevice);
    assert(res == VK_SUCCESS);
}

void phx::gfx::platform::VulkanGpuDevice::ResizeSwapChain(SwapChainDesc const& swapChainDesc)
{
    std::scoped_lock _(m_swapchainMutex);
    m_swapChainDesc = swapChainDesc;

}

DynamicMemoryPage phx::gfx::platform::VulkanGpuDevice::AllocateDynamicMemoryPage(size_t pageSize)
{
    return m_dynamicAllocator.Allocate(pageSize);
}

ShaderHandle phx::gfx::platform::VulkanGpuDevice::CreateShader(ShaderDesc const& desc)
{
    Handle<Shader> retVal = this->m_shaderPool.Emplace();
    Shader_VK& impl = *this->m_shaderPool.Get(retVal);

    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = desc.ByteCode.Size();
    moduleInfo.pCode = (const uint32_t*)desc.ByteCode.begin();
    VkResult res = vkCreateShaderModule(m_vkDevice, &moduleInfo, nullptr, &impl.ShaderModule);
    assert(res == VK_SUCCESS);

    impl.StageInfo = {};
    impl.StageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    impl.StageInfo.module = impl.ShaderModule;
    impl.StageInfo.pName = desc.EntryPoint;

    switch (desc.Stage)
    {
    case ShaderStage::MS:
        impl.StageInfo.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
        break;
    case ShaderStage::AS:
        impl.StageInfo.stage = VK_SHADER_STAGE_TASK_BIT_EXT;
        break;
    case ShaderStage::VS:
        impl.StageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        break;
    case ShaderStage::HS:
        impl.StageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        break;
    case ShaderStage::DS:
        impl.StageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        break;
    case ShaderStage::GS:
        impl.StageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        break;
    case ShaderStage::PS:
        impl.StageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        break;
    case ShaderStage::CS:
        impl.StageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        break;
    default:
        // also means library shader (ray tracing)
        impl.StageInfo.stage = VK_SHADER_STAGE_ALL;
        break;
    }

    {
        SpvReflectShaderModule module;
        SpvReflectResult result = spvReflectCreateShaderModule(moduleInfo.codeSize, moduleInfo.pCode, &module);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        spvReflectDestroyShaderModule(&module);
    }

    return retVal;
}

void phx::gfx::platform::VulkanGpuDevice::DeleteShader(ShaderHandle handle)
{
    DeferredItem d =
    {
        m_frameCount,
        [=]()
        {
            Shader_VK* impl = m_shaderPool.Get(handle);
            if (impl)
            {
                vkDestroyShaderModule(m_vkDevice, impl->ShaderModule, nullptr);
                m_shaderPool.Release(handle);
            }
        }
    };

    m_deferredQueue.push_back(d);
}

PipelineStateHandle phx::gfx::platform::VulkanGpuDevice::CreatePipeline(PipelineStateDesc2 const& desc, RenderPassInfo* renderPassInfo)
{
    Handle<PipelineState> retVal = this->m_pipelineStatePool.Emplace();
    PipelineState_Vk& impl = *this->m_pipelineStatePool.Get(retVal);

    // TODO: We can easly hash the shaders here since it's just a uint32 handle


    // -- TODO: Process reflection data here ---
    // -- TODO: Bindless ---
    // Cache Pipeline Layouts

    {
        // -- Create Pipeline layout ---
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;       // No descriptor sets
        pipelineLayoutInfo.pSetLayouts = nullptr;    // No layouts
        pipelineLayoutInfo.pushConstantRangeCount = 0; // No push constants
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        VkResult result = vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutInfo, nullptr, &impl.PipelineLayout);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    //pipelineInfo.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = impl.PipelineLayout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    // -- Shader Stages ---
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(static_cast<size_t>(ShaderStage::Count));

    if (desc.MS.IsValid())
    {
        Shader_VK* impl = m_shaderPool.Get(desc.MS);
        shaderStages.push_back(impl->StageInfo);
    }
    if (desc.AS.IsValid())
    {
        Shader_VK* impl = m_shaderPool.Get(desc.AS);
        shaderStages.push_back(impl->StageInfo);;
    }
    if (desc.VS.IsValid())
    {
        Shader_VK* impl = m_shaderPool.Get(desc.VS);
        shaderStages.push_back(impl->StageInfo);
    }
    if (desc.HS.IsValid())
    {
        Shader_VK* impl = m_shaderPool.Get(desc.HS);
        shaderStages.push_back(impl->StageInfo);
    }
    if (desc.DS.IsValid())
    {
        Shader_VK* impl = m_shaderPool.Get(desc.DS);
        shaderStages.push_back(impl->StageInfo);
    }
    if (desc.GS.IsValid())
    {
        Shader_VK* impl = m_shaderPool.Get(desc.GS);
        shaderStages.push_back(impl->StageInfo);
    }
    if (desc.PS.IsValid())
    {
        Shader_VK* impl = m_shaderPool.Get(desc.PS);
        shaderStages.push_back(impl->StageInfo);
    }

    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();


    // -- Primitive type ---
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    switch (desc.PrimType)
    {
    case PrimitiveType::PointList:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;
    case PrimitiveType::LineList:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case PrimitiveType::LineStrip:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        break;
    case PrimitiveType::TriangleStrip:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        break;
    case PrimitiveType::TriangleList:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    case PrimitiveType::PatchList:
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        break;
    default:
        break;
    }
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	pipelineInfo.pInputAssemblyState = &inputAssembly;

	// -- Rasterizer ---
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_TRUE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

    const void** tail = &rasterizer.pNext;

    // depth clip will be enabled via Vulkan 1.1 extension VK_EXT_depth_clip_enable:
    VkPipelineRasterizationDepthClipStateCreateInfoEXT depthClipStateInfo = {};
    depthClipStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
    depthClipStateInfo.depthClipEnable = VK_TRUE;

    VkPhysicalDeviceDepthClipEnableFeaturesEXT depthClipEnableFeature = {};
    depthClipEnableFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
    *tail = &depthClipStateInfo;
    tail = &depthClipStateInfo.pNext;

    if (desc.RasterRenderState != nullptr)
    {
        const RasterRenderState& rs = *desc.RasterRenderState;

        switch (rs.FillMode)
        {
        case RasterFillMode::Wireframe:
            rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
            break;
        case RasterFillMode::Solid:
        default:
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            break;
        }

        switch (rs.CullMode)
        {
        case RasterCullMode::Back:
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            break;
        case RasterCullMode::Front:
            rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
            break;
        case RasterCullMode::None:
        default:
            rasterizer.cullMode = VK_CULL_MODE_NONE;
            break;
        }

        rasterizer.frontFace = rs.FrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = rs.DepthBias != 0 || rs.SlopeScaledDepthBias != 0;
        rasterizer.depthBiasConstantFactor = static_cast<float>(rs.DepthBias);
        rasterizer.depthBiasClamp = rs.DepthBiasClamp;
        rasterizer.depthBiasSlopeFactor = rs.SlopeScaledDepthBias;

        // Depth clip will be enabled via Vulkan 1.1 extension VK_EXT_depth_clip_enable:
        depthClipStateInfo.depthClipEnable = rs.DepthClipEnable ? VK_TRUE : VK_FALSE;

        VkPipelineRasterizationConservativeStateCreateInfoEXT rasterizationConservativeState = {};
        if (/*CheckCapability(GraphicsDeviceCapability::CONSERVATIVE_RASTERIZATION) && */rs.ConservativeRasterEnable)
        {
            rasterizationConservativeState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
            rasterizationConservativeState.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
            rasterizationConservativeState.extraPrimitiveOverestimationSize = 0.0f;
            *tail = &rasterizationConservativeState;
            tail = &rasterizationConservativeState.pNext;
        }
	}
	pipelineInfo.pRasterizationState = &rasterizer;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 0;
	viewportState.pViewports = nullptr;
	viewportState.scissorCount = 0;
	viewportState.pScissors = nullptr;

	pipelineInfo.pViewportState = &viewportState;

	// -- Depth-Stencil ---
	VkPipelineDepthStencilStateCreateInfo depthstencil = {};
	depthstencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    if (desc.DepthStencilRenderState)
	{
        DepthStencilRenderState* dss = desc.DepthStencilRenderState;
		depthstencil.depthTestEnable = dss->DepthEnable ? VK_TRUE : VK_FALSE;
		depthstencil.depthWriteEnable = dss->DepthWriteMask == DepthWriteMask::Zero ? VK_FALSE : VK_TRUE;
		depthstencil.depthCompareOp = ConvertComparisonFunc(dss->DepthFunc);

		if (dss->StencilEnable)
		{
			depthstencil.stencilTestEnable = VK_TRUE;

			depthstencil.front.compareMask = dss->StencilReadMask;
			depthstencil.front.writeMask = dss->StencilWriteMask;
			depthstencil.front.reference = 0; // runtime supplied
			depthstencil.front.compareOp = ConvertComparisonFunc(dss->FrontFace.StencilFunc);
			depthstencil.front.passOp = ConvertStencilOp(dss->FrontFace.StencilPassOp);
			depthstencil.front.failOp = ConvertStencilOp(dss->FrontFace.StencilFailOp);
			depthstencil.front.depthFailOp = ConvertStencilOp(dss->FrontFace.StencilDepthFailOp);

			depthstencil.back.compareMask = dss->StencilReadMask;
			depthstencil.back.writeMask = dss->StencilWriteMask;
			depthstencil.back.reference = 0; // runtime supplied
			depthstencil.back.compareOp = ConvertComparisonFunc(dss->BackFace.StencilFunc);
			depthstencil.back.passOp = ConvertStencilOp(dss->BackFace.StencilPassOp);
			depthstencil.back.failOp = ConvertStencilOp(dss->BackFace.StencilFailOp);
			depthstencil.back.depthFailOp = ConvertStencilOp(dss->BackFace.StencilDepthFailOp);
		}
		else
		{
			depthstencil.stencilTestEnable = VK_FALSE;

			depthstencil.front.compareMask = 0;
			depthstencil.front.writeMask = 0;
			depthstencil.front.reference = 0;
			depthstencil.front.compareOp = VK_COMPARE_OP_NEVER;
			depthstencil.front.passOp = VK_STENCIL_OP_KEEP;
			depthstencil.front.failOp = VK_STENCIL_OP_KEEP;
			depthstencil.front.depthFailOp = VK_STENCIL_OP_KEEP;

			depthstencil.back.compareMask = 0;
			depthstencil.back.writeMask = 0;
			depthstencil.back.reference = 0; // runtime supplied
			depthstencil.back.compareOp = VK_COMPARE_OP_NEVER;
			depthstencil.back.passOp = VK_STENCIL_OP_KEEP;
			depthstencil.back.failOp = VK_STENCIL_OP_KEEP;
			depthstencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
		}

#if false
		if (CheckCapability(GraphicsDeviceCapability::DEPTH_BOUNDS_TEST))
		{
			depthstencil.depthBoundsTestEnable = dss->DepthBoundsTestEnable ? VK_TRUE : VK_FALSE;
		}
		else
		{
			depthstencil.depthBoundsTestEnable = VK_FALSE;
		}
#else

		depthstencil.depthBoundsTestEnable = VK_FALSE;
#endif
	}

	pipelineInfo.pDepthStencilState = &depthstencil;

	// -- Tessellation ---
#if false
	VkPipelineTessellationStateCreateInfo tessellationInfo = {};
	tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessellationInfo.patchControlPoints = desc->patch_control_points;

	pipelineInfo.pTessellationState = &tessellationInfo;
#endif

    // -- Dynamic States ---
    if (!desc.MS.IsValid())
    {
        pipelineInfo.pDynamicState = &m_dynamicStateInfo;
    }
    else
    {
        pipelineInfo.pDynamicState = &m_dynamicStateInfo_MeshShader;
    }

    // -- Input Layout ---
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    if (desc.InputLayout)
    {
        InputLayout* inputLayout = desc.InputLayout;
        uint32_t lastBinding = 0xFFFFFFFF;
        for (auto& e : inputLayout->elements)
        {
            if (e.InputSlot == lastBinding)
                continue;
            lastBinding = e.InputSlot;
            VkVertexInputBindingDescription& bind = bindings.emplace_back();
            bind.binding = e.InputSlot;
            bind.inputRate = e.InputSlotClass== InputClassification::PerVertexData? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
            bind.stride = GetFormatStride(e.Format);
        }

        uint32_t offset = 0;
        uint32_t i = 0;
        lastBinding = 0xFFFFFFFF;
        for (auto& e : inputLayout->elements)
        {
            VkVertexInputAttributeDescription attr = {};
            attr.binding = e.InputSlot;
            if (attr.binding != lastBinding)
            {
                lastBinding = attr.binding;
                offset = 0;
            }
            attr.format = FormatToVkFormat(e.Format);
            attr.location = i;
            attr.offset = e.AlignedByteOffset;
            if (attr.offset == InputLayout::APPEND_ALIGNED_ELEMENT)
            {
                // need to manually resolve this from the format spec.
                attr.offset = offset;
                offset += GetFormatStride(e.Format);
            }

            attributes.push_back(attr);

            i++;
        }

        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
        vertexInputInfo.pVertexBindingDescriptions = bindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
	}

	pipelineInfo.pVertexInputState = &vertexInputInfo;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional
	pipelineInfo.pMultisampleState = &multisampling;

	VkResult res = vkCreateGraphicsPipelines(m_vkDevice, m_vkPipelineCache, 1, &pipelineInfo, nullptr, &impl.Pipeline);
	assert(res == VK_SUCCESS);
    return retVal;
}

void phx::gfx::platform::VulkanGpuDevice::DeletePipeline(PipelineStateHandle handle)
{
    DeferredItem d =
    {
        m_frameCount,
        [=]()
        {
            PipelineState_Vk* impl = m_pipelineStatePool.Get(handle);
            if (impl)
            {
                vkDestroyPipeline(m_vkDevice, impl->Pipeline, nullptr);

                vkDestroyPipelineLayout(m_vkDevice, impl->PipelineLayout, nullptr);

                m_pipelineStatePool.Release(handle);
            }
        }
    };

    m_deferredQueue.push_back(d);
}

BufferHandle phx::gfx::platform::VulkanGpuDevice::CreateBuffer(BufferDesc const& desc)
{
    Handle<Buffer> retVal = this->m_bufferPool.Emplace();
    Buffer_VK& impl = *this->m_bufferPool.Get(retVal);

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.SizeInBytes;
    bufferInfo.usage = 0;

    static const std::vector <std::pair<BindingFlags, VkBufferUsageFlags>> kUsageMapping =
    {
        { BindingFlags::VertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
        { BindingFlags::IndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
        { BindingFlags::ConstantBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
        { BindingFlags::ShaderResource, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT},
        { BindingFlags::UnorderedAccess, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT},
    };

    for (const auto& [flag, usageFlag] : kUsageMapping)
    {
        if (EnumHasAnyFlags(desc.Binding, flag))
        {
            bufferInfo.usage |= usageFlag;
        }
    }

    // Misc Flags
    static const std::vector <std::pair<BufferMiscFlags, VkBufferUsageFlags>> kUsageMappingMisc =
    {
        { BufferMiscFlags::Raw, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
        { BufferMiscFlags::Structured, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
        { BufferMiscFlags::IndirectArgs, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT},
        { BufferMiscFlags::RayTracing, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR },
    };

    for (const auto& [flag, usageFlag] : kUsageMappingMisc)
    {
        if (EnumHasAnyFlags(desc.MiscFlags, flag))
        {
            bufferInfo.usage |= usageFlag;
        }
    }

    if (m_vulkan12Features.bufferDeviceAddress == VK_TRUE)
    {
        bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    bufferInfo.flags = 0;

    if (m_queueFamilies.AreSeperate())
    {
        bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

        Span<uint32_t> families = m_queueFamilies.GetFamilies();
        bufferInfo.queueFamilyIndexCount = (uint32_t)families.Size();
        bufferInfo.pQueueFamilyIndices = families.begin();
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    if (EnumHasAnyFlags(desc.MiscFlags, BufferMiscFlags::IsAliasedResource))
    {
        // TOD:
    }
    else if(EnumHasAnyFlags(desc.MiscFlags, BufferMiscFlags::Sparse))
    {
    }
    else
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        if (desc.Usage == Usage::ReadBack)
        {
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }
        else if (desc.Usage == Usage::Upload)
        {
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }

        VkResult res = VK_SUCCESS;
        if (desc.Alias == nullptr)
        {
             res = vmaCreateBuffer(m_vmaAllocator, &bufferInfo, &allocInfo, &impl.BufferVk, &impl.Allocation, nullptr);
        }
        else
        {
            // Aliasing: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/resource_aliasing.html
            if (std::holds_alternative<TextureHandle>(desc.Alias->Handle))
            {
                Texture_VK* aliasTexture = m_texturePool.Get(std::get<TextureHandle>(desc.Alias->Handle));
                res = vmaCreateAliasingBuffer2(
                    m_vmaAllocator,
                    aliasTexture->Allocation,
                    desc.Alias->AliasOffset,
                    &bufferInfo,
                    &impl.BufferVk);
            }
            else
            {
                Buffer_VK* aliasBuffer = m_bufferPool.Get(std::get<BufferHandle>(desc.Alias->Handle));
                assert(aliasBuffer);
                res = vmaCreateAliasingBuffer2(
                    m_vmaAllocator,
                    aliasBuffer->Allocation,
                    desc.Alias->AliasOffset,
                    &bufferInfo,
                    &impl.BufferVk);

            }
		}

		assert(res == VK_SUCCESS);

#ifdef _DEBUG
		// Now you have allocInfo.memoryType, which tells you which memory type was used
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &memoryProperties);

		// Use the memoryTypeIndex to find the memory type
		VkMemoryType memoryType = memoryProperties.memoryTypes[impl.Allocation->GetMemoryTypeIndex()];

		// Find the corresponding heap
		uint32_t heapIndex = memoryType.heapIndex;
		VkMemoryHeap heap = memoryProperties.memoryHeaps[heapIndex];

		VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
		vmaGetHeapBudgets(m_vmaAllocator, budgets);

        PHX_CORE_INFO("Created Buffer on {0} - {1}/{2}", heapIndex, budgets[heapIndex].usage, heap.size);
#endif
    }

    // TODO Set up Mappings

    if (desc.Usage == Usage::ReadBack || desc.Usage == Usage::Upload)
    {
        impl.MappedData = impl.Allocation->GetMappedData();
        impl.MappedSizeInBytes = impl.Allocation->GetSize();
    }

    if (bufferInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkBufferDeviceAddressInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = impl.BufferVk;
        impl.GpuAddress = vkGetBufferDeviceAddress(m_vkDevice, &info);
    }

    // TODO Upload Data

    if ((desc.Binding & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
    {
        CreateSubresource(impl, desc, SubresouceType::SRV, 0u);
    }

    if ((desc.Binding & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
    {
        CreateSubresource(impl, desc, SubresouceType::UAV, 0u);
    }

    return retVal;
}

void phx::gfx::platform::VulkanGpuDevice::DeleteBuffer(BufferHandle handle)
{
    DeferredItem d =
    {
        m_frameCount,
        [=]()
        {
            Buffer_VK* impl = m_bufferPool.Get(handle);
            // TODO: Move into the deconstructor of struct
            if (impl)
            {
                if (impl->Srv.IsValid())
                {
                    if (impl->Srv.IsTyped)
                    {
                        m_bindlessUniformTexelBuffers.Free(impl->Srv.Index);
                    }
                    else
                    {
                        m_bindlessStorageBuffers.Free(impl->Srv.Index);
                    }

                    if (impl->Srv.ViewVk != VK_NULL_HANDLE)
                        vkDestroyBufferView(m_vkDevice, impl->Uav.ViewVk, nullptr);
                    impl->Srv = {};
                }
                if (impl->Uav.IsValid())
                {
                    if (impl->Uav.IsTyped)
                    {
                        m_bindlessStorageTexelBuffers.Free(impl->Uav.Index);
                    }
                    else
                    {
                        m_bindlessStorageBuffers.Free(impl->Uav.Index);
                    }

                    if (impl->Uav.ViewVk != VK_NULL_HANDLE)
                        vkDestroyBufferView(m_vkDevice, impl->Uav.ViewVk, nullptr);
                    impl->Uav = {};
                }
                vmaDestroyBuffer(m_vmaAllocator, impl->BufferVk, impl->Allocation);
            }
            m_bufferPool.Release(handle);
        }
    };

    m_deferredQueue.push_back(d);
}

TextureHandle phx::gfx::platform::VulkanGpuDevice::CreateTexture(TextureDesc const& desc, SubresourceData* initData)
{
    Handle<Texture> retVal = this->m_texturePool.Emplace();
    Texture_VK& impl = *this->m_texturePool.Get(retVal);

	VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.extent.width = desc.Width;
    imageInfo.extent.height = desc.Height;
    imageInfo.extent.depth = desc.Depth;
	imageInfo.format = FormatToVkFormat(desc.Format);
	imageInfo.arrayLayers = desc.ArraySize;
    imageInfo.mipLevels = desc.MipLevels;
    imageInfo.samples = (VkSampleCountFlagBits)desc.SampleCount;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = 0;

    static const std::vector <std::pair<BindingFlags, VkImageUsageFlags>> kUsageMapping =
    {
        { BindingFlags::ShaderResource, VK_IMAGE_USAGE_SAMPLED_BIT},
        { BindingFlags::UnorderedAccess, VK_IMAGE_USAGE_STORAGE_BIT},
        { BindingFlags::RenderTarget, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
        { BindingFlags::DepthStencil, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT},
        { BindingFlags::ShadingRate, VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR},
    };

    for (const auto& [flag, usageFlag] : kUsageMapping)
    {
        if (EnumHasAnyFlags(desc.BindingFlags, flag))
        {
            imageInfo.usage |= usageFlag;
        }
    }

    if (EnumHasAnyFlags(desc.BindingFlags, BindingFlags::UnorderedAccess))
    {
        if (IsFormatSRGB(desc.Format))
        {
            imageInfo.flags |= VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;
        }
    }

    // Misc Flags
    static const std::vector <std::pair<TextureMiscFlags, VkImageUsageFlags>> kUsageMappingMisc =
    {
        { TextureMiscFlags::TransientAttachment, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT},
        { TextureMiscFlags::TypedFormatCasting, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT},
        { TextureMiscFlags::TypelessFormatCasting, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT},
    };

    for (const auto& [flag, usageFlag] : kUsageMappingMisc)
    {
        if (EnumHasAnyFlags(desc.MiscFlags, flag))
        {
            imageInfo.usage |= usageFlag;
        }
    }

    if (desc.Dimension == TextureDimension::TextureCube || desc.Dimension == TextureDimension::TextureCubeArray)
    {
        imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    if (!EnumHasAnyFlags(desc.MiscFlags, TextureMiscFlags::TransientAttachment))
    {
        imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    if (m_queueFamilies.AreSeperate())
    {
        imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

        Span<uint32_t> families = m_queueFamilies.GetFamilies();
        imageInfo.queueFamilyIndexCount = (uint32_t)families.Size();
        imageInfo.pQueueFamilyIndices = families.begin();
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    switch (desc.Dimension)
    {
    case TextureDimension::Texture1D:
    case TextureDimension::Texture1DArray:
        imageInfo.imageType = VK_IMAGE_TYPE_1D;
        break;
    case TextureDimension::Texture2D:
    case TextureDimension::Texture2DArray:
    case TextureDimension::TextureCube:
    case TextureDimension::TextureCubeArray:
    case TextureDimension::Texture2DMS:
    case TextureDimension::Texture2DMSArray:
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        break;
    case TextureDimension::Texture3D:
        imageInfo.imageType = VK_IMAGE_TYPE_3D;
        break;
    default:
        assert(0);
        break;
    }

    VkResult res;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    // TODO: Sparse Textures
    if (EnumHasAnyFlags(desc.MiscFlags, TextureMiscFlags::Sparse))
    {
    }
    else
    {
        // TODO: Support image read backs / uploads

        if (desc.Alias == nullptr)
        {
            res = vmaCreateImage(
                m_vmaAllocator,
                &imageInfo,
                &allocInfo,
                &impl.ImageVk,
                &impl.Allocation,
                nullptr);
        }
        else
        {
            // Aliasing: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/resource_aliasing.html
            if (std::holds_alternative<TextureHandle>(desc.Alias->Handle))
            {
                Texture_VK* aliasTexture = m_texturePool.Get(std::get<TextureHandle>(desc.Alias->Handle));
                res = vmaCreateAliasingImage2(
                    m_vmaAllocator,
                    aliasTexture->Allocation,
                    desc.Alias->AliasOffset,
                    &imageInfo,
                    &impl.ImageVk);
            }
            else
            {
                Buffer_VK* aliasBuffer = m_bufferPool.Get(std::get<BufferHandle>(desc.Alias->Handle));
                assert(aliasBuffer);
                res = vmaCreateAliasingImage2(
                    m_vmaAllocator,
                    aliasBuffer->Allocation,
                    desc.Alias->AliasOffset,
                    &imageInfo,
                    &impl.ImageVk);
            }
        }

        assert(res == VK_SUCCESS);
    }

    if (initData)
    {
        CopyCtxManager::Ctx ctx = m_copyCtxManager.Begin(impl.Allocation->GetSize());
        void* mappedData = ctx.MappedData;

        std::vector<VkBufferImageCopy> copyRegions;

        VkDeviceSize copyOffset = 0;
        uint32_t initDataIdx = 0;
        for (uint32_t layer = 0; layer < desc.ArraySize; ++layer)
        {
            uint32_t width = imageInfo.extent.width;
            uint32_t height = imageInfo.extent.height;
            uint32_t depth = imageInfo.extent.depth;
            for (uint32_t mip = 0; mip < desc.MipLevels; mip++)
            {
                const SubresourceData& subresourceData = initData[initDataIdx++];
                const uint32_t blockSize = GetFormatBlockSize(desc.Format);
                const uint32_t numBlocksX = std::max(1u, width / blockSize);
                const uint32_t numBlocksY = std::max(1u, height / blockSize);
                const uint32_t dstRowPitch = numBlocksX * GetFormatStride(desc.Format);
                const uint32_t dstSlicePitch = dstRowPitch * numBlocksY;
                const uint32_t srcRowPitch = subresourceData.rowPitch;
                const uint32_t srcSlicePitch = subresourceData.slicePitch;
                for (uint32_t z = 0; z < depth; ++z)
                {
                    uint8_t* dstSlice = (uint8_t*)mappedData + copyOffset + dstSlicePitch * z;
                    uint8_t* srcSlice = (uint8_t*)subresourceData.pData + srcSlicePitch * z;
                    for (uint32_t y = 0; y < numBlocksY; ++y)
                    {
                        std::memcpy(
                            dstSlice + dstRowPitch * y,
                            srcSlice + srcRowPitch * y,
                            dstRowPitch);
                    }
                }

                assert(ctx.IsValid());
                VkBufferImageCopy copyRegion = {};
                copyRegion.bufferOffset = copyOffset;
                copyRegion.bufferRowLength = 0;
                copyRegion.bufferImageHeight = 0;

                copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.imageSubresource.mipLevel = mip;
                copyRegion.imageSubresource.baseArrayLayer = layer;
                copyRegion.imageSubresource.layerCount = 1;

                copyRegion.imageOffset = { 0, 0, 0 };
                copyRegion.imageExtent = {
                    width,
                    height,
                    depth };

                copyRegions.push_back(copyRegion);

                copyOffset += dstSlicePitch * depth;

                // fix for validation: on transfer queue the srcOffset must be 4-byte aligned
                copyOffset = MemoryAlign(copyOffset, VkDeviceSize(4));

                width = std::max(1u, width / 2);
                height = std::max(1u, height / 2);
                depth = std::max(1u, depth / 2);
            }
        }

        VkImageMemoryBarrier2 barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.image = impl.ImageVk;
        barrier.oldLayout = imageInfo.initialLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.srcAccessMask = 0;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        VkDependencyInfo dependencyInfo = {};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(ctx.TransferCommandBuffer, &dependencyInfo);

        Buffer_VK* staggingBuffer = m_bufferPool.Get(ctx.UploadBuffer);
        vkCmdCopyBufferToImage(
            ctx.TransferCommandBuffer,
            staggingBuffer->BufferVk,
            impl.ImageVk,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            (uint32_t)copyRegions.size(),
            copyRegions.data()
        );

        std::swap(barrier.srcStageMask, barrier.dstStageMask);
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = ConvertImageLayout(desc.InitialState);
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = _ParseResourceState(desc.InitialState);
        vkCmdPipelineBarrier2(ctx.TransferCommandBuffer, &dependencyInfo);

        m_copyCtxManager.Submit(ctx);
    }

    if ((desc.BindingFlags & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
    {
        CreateSubresource(impl, desc, SubresouceType::SRV, 0, ~0u, 0, ~0u);
    }

    if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
    {
        CreateSubresource(impl, desc, SubresouceType::RTV, 0, ~0u, 0, ~0u);
    }

    if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
    {
        CreateSubresource(impl, desc, SubresouceType::DSV, 0, ~0u, 0, ~0u);
    }

    if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
    {
        CreateSubresource(impl, desc, SubresouceType::UAV, 0, ~0u, 0, ~0u);
    }
    return retVal;
}

void phx::gfx::platform::VulkanGpuDevice::DeleteTexture(TextureHandle handle)
{
    DeferredItem d =
    {
        m_frameCount,
        [=]()
        {
            
            Texture_VK* impl = m_texturePool.Get(handle);
            // TODO: Move into the deconstructor of struct
            if (impl)
            {
                if (impl->Srv.IsValid())
                {
                    m_bindlessSampledImages.Free(impl->Srv.Index);
                    impl->Srv = {};
                }
                if (impl->Uav.IsValid())
                {
                    m_bindlessStorageImages.Free(impl->Uav.Index);
                    impl->Uav = {};
                }
                if (impl->Rtv.ViewVk != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(m_vkDevice, impl->Rtv.ViewVk, nullptr);
                    impl->Rtv = {};
                }
                if (impl->Dsv.ViewVk != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(m_vkDevice, impl->Dsv.ViewVk, nullptr);
                    impl->Dsv = {};
                }

                vmaDestroyImage(m_vmaAllocator, impl->ImageVk, impl->Allocation);
                m_texturePool.Release(handle);
            }
        }
    };

    m_deferredQueue.push_back(d);
}

void* phx::gfx::platform::VulkanGpuDevice::GetMappedData(BufferHandle handle)
{
    Buffer_VK* impl = m_bufferPool.Get(handle);
    return impl ? impl->MappedData : nullptr;
}

DescriptorIndex VulkanGpuDevice::GetDescriptorIndex(TextureHandle handle, SubresouceType type, int subResource)
{
    Texture_VK* impl = m_texturePool.Get(handle);
    if (impl == nullptr)
        return cInvalidDescriptorIndex;

    // TODO:
    return cInvalidDescriptorIndex;
}

DescriptorIndex VulkanGpuDevice::GetDescriptorIndex(BufferHandle handle, SubresouceType type, int subResource)
{
    Buffer_VK* impl = m_bufferPool.Get(handle);
    if (impl == nullptr)
        return cInvalidDescriptorIndex;

    // TODO:
    return cInvalidDescriptorIndex;
}

int phx::gfx::platform::VulkanGpuDevice::CreateSubresource(Texture_VK& texture, TextureDesc const& desc, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount)
{
    VkImageViewCreateInfo viewDesc = {};
    viewDesc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewDesc.flags = 0;
    viewDesc.image = texture.ImageVk;
    viewDesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    viewDesc.subresourceRange.baseArrayLayer = firstSlice;
    viewDesc.subresourceRange.layerCount = sliceCount;
    viewDesc.subresourceRange.baseMipLevel = firstMip;
    viewDesc.subresourceRange.levelCount = mipCount;

    // TODO Support Swizzle?

    if (subresourceType == SubresouceType::SRV)
    {
        Swizzle defaultSwizzle = {};
        viewDesc.components = ConvertSwizzle(defaultSwizzle);
    }

    viewDesc.format = FormatToVkFormat(desc.Format);


    switch (desc.Dimension)
    {
    case TextureDimension::Texture1D:
    case TextureDimension::Texture1DArray:
    {
        if (desc.ArraySize > 1)
        {
            viewDesc.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        }
        else
        {
            viewDesc.viewType = VK_IMAGE_VIEW_TYPE_1D;
        }
        break;
    }
    case TextureDimension::Texture2D:
    case TextureDimension::Texture2DArray:
    {
        if (desc.ArraySize > 1)
        {
            viewDesc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        }
        else
        {
            viewDesc.viewType = VK_IMAGE_VIEW_TYPE_2D;
        }
        break;
    }
    case TextureDimension::TextureCube:
    {
        if (desc.ArraySize > 1)
        {
            if (desc.ArraySize > 6 && sliceCount > 6)
            {
                viewDesc.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            }
            else
            {
                viewDesc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            }
        }
        break;
    };
    case TextureDimension::Texture3D:
    {
        viewDesc.viewType = VK_IMAGE_VIEW_TYPE_3D;
        break;
    }
    case TextureDimension::Texture2DMS:
    case TextureDimension::Texture2DMSArray:
    case TextureDimension::Unknown:
    default:
        throw std::runtime_error("Unsupported Enum");
    }


    switch (subresourceType)
    {
    case SubresouceType::SRV:
    {
        if (IsFormatDepthSupport(desc.Format))
        {
            viewDesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        // This is required in cases where image was created with eg. USAGE_STORAGE, but
        //	the view format that we are creating doesn't support USAGE_STORAGE (for examplle: SRGB formats)
        VkImageViewUsageCreateInfo viewUsageInfo = {};
        viewUsageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
        viewUsageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        viewDesc.pNext = &viewUsageInfo;

#if false
        VkImageViewMinLodCreateInfoEXT min_lod_info = {};
        min_lod_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_MIN_LOD_CREATE_INFO_EXT;
        min_lod_info.minLod = 0;
        if (min_lod_info.minLod > 0 && image_view_min_lod_features.minLod == VK_TRUE)
        {
            viewUsageInfo.pNext = &min_lod_info;
        }
#endif
        VkResult res = vkCreateImageView(m_vkDevice, &viewDesc, nullptr, &texture.Srv.ViewVk);

        texture.Srv.Index = m_bindlessSampledImages.Allocate();
        if (texture.Srv.Index != cInvalidDescriptorIndex)
        {
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageView = texture.Srv.ViewVk;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.dstBinding = 0;
            write.dstArrayElement = texture.Srv.Index;
            write.descriptorCount = 1;
            write.dstSet = m_bindlessSampledImages.DescritporSetVk;
            write.pImageInfo = &imageInfo;
            vkUpdateDescriptorSets(m_vkDevice, 1, &write, 0, nullptr);
        }

        assert(res == VK_SUCCESS);
    }
    break;
    case SubresouceType::UAV:
    {
        if (viewDesc.viewType == VK_IMAGE_VIEW_TYPE_CUBE || viewDesc.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
        {
            viewDesc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        }

        VkResult res = vkCreateImageView(m_vkDevice, &viewDesc, nullptr, &texture.Uav.ViewVk);

        texture.Uav.Index = m_bindlessStorageImages.Allocate();
        if (texture.Uav.Index != cInvalidDescriptorIndex)
        {
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageView = texture.Uav.ViewVk;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            write.dstBinding = 0;
            write.dstArrayElement = texture.Uav.Index;
            write.descriptorCount = 1;
            write.dstSet = m_bindlessStorageBuffers.DescritporSetVk;
            write.pImageInfo = &imageInfo;
            vkUpdateDescriptorSets(m_vkDevice, 1, &write, 0, nullptr);
        }

        assert(res == VK_SUCCESS);
    }
    break;
    case SubresouceType::RTV:
    {
        viewDesc.subresourceRange.levelCount = 1;
        VkResult res = vkCreateImageView(m_vkDevice, &viewDesc, nullptr, &texture.Rtv.ViewVk);
        assert(res == VK_SUCCESS);
    }
    break;
    case SubresouceType::DSV:
    {
        viewDesc.subresourceRange.levelCount = 1;
        viewDesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        VkResult res = vkCreateImageView(m_vkDevice, &viewDesc, nullptr, &texture.Dsv.ViewVk);
        assert(res == VK_SUCCESS);
    }
    break;
    default:
        break;
    }
    return -1;
}

int phx::gfx::platform::VulkanGpuDevice::CreateSubresource(Buffer_VK& buffer, BufferDesc const& desc, SubresouceType subresourceType, size_t offset, size_t size)
{
    assert(subresourceType == SubresouceType::SRV || subresourceType == SubresouceType::UAV);

    Format format = desc.Format;

    Buffer_VK::BufferView& view = subresourceType == SubresouceType::SRV 
        ? buffer.Srv
        : buffer.Uav;
    

    // Is raw buffer
    if (format == Format::UNKNOWN)
    {
        view.IsTyped = false;
        view.Index = m_bindlessStorageBuffers.Allocate();

        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = buffer.BufferVk;
        bufferInfo.offset = offset;
        bufferInfo.range = size;

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.dstBinding = 0;
        write.dstArrayElement = view.Index;
        write.descriptorCount = 1;
        write.dstSet = m_bindlessStorageBuffers.DescritporSetVk;
        write.pBufferInfo = &bufferInfo;
    }
    else
    {
        // Typed buffer
        view.IsTyped = true;

        VkBufferViewCreateInfo srvDesc = {};
        srvDesc.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        srvDesc.buffer = buffer.BufferVk;
        srvDesc.flags = 0;
        srvDesc.format = FormatToVkFormat(format);
        srvDesc.offset = offset;
        srvDesc.range = std::min(size, (uint64_t)desc.SizeInBytes- srvDesc.offset);

        VkResult res = vkCreateBufferView(m_vkDevice, &srvDesc, nullptr, &view.ViewVk);
        assert(res == VK_SUCCESS);

        if (subresourceType == SubresouceType::SRV)
        {
            view.Index = m_bindlessUniformTexelBuffers.Allocate();
            if (view.IsValid())
            {
                VkWriteDescriptorSet write = {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                write.dstBinding = 0;
                write.dstArrayElement = view.Index;
                write.descriptorCount = 1;
                write.dstSet = m_bindlessUniformTexelBuffers.DescritporSetVk;
                write.pTexelBufferView = &view.ViewVk;
                vkUpdateDescriptorSets(m_vkDevice, 1, &write, 0, nullptr);
            }

            return -1;
        }
        else
        {
            view.Index = m_bindlessStorageTexelBuffers.Allocate();
            if (view.IsValid())
            {
                VkWriteDescriptorSet write = {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                write.dstBinding = 0;
                write.dstArrayElement = view;
                write.descriptorCount = 1;
                write.dstSet = m_bindlessStorageTexelBuffers.DescritporSetVk;
                write.pTexelBufferView = &view.ViewVk;
                vkUpdateDescriptorSets(m_vkDevice, 1, &write, 0, nullptr);
            }
            return -1;
        }
    }
}

void phx::gfx::platform::VulkanGpuDevice::CreateInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "PHX Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> instanceExtensions = m_extManager.GetOptionalExtensions(kOptionalExtensions);
    
    m_extManager.CheckRequiredExtensions(kRequiredExtensions);

    for (const char* extName : kRequiredExtensions)
    {
        instanceExtensions.push_back(extName);
    }

    for (const char* extName : instanceExtensions)
    {
        PHX_CORE_INFO("[Vulkan] - Enabling Vulkan Extension '{0}'", extName);
    }

    createInfo.enabledExtensionCount = instanceExtensions.size();
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    createInfo.enabledLayerCount = 0;
    std::vector<const char*> instanceLayers;
    std::vector<const char*> enabledLayers;
    for (auto& availableLayers : kValidationLayerPriorityList)
    {
        enabledLayers = m_layerManager.GetOptionalLayers(availableLayers);
        if (!enabledLayers.empty())
        {
            createInfo.enabledLayerCount = enabledLayers.size();
            createInfo.ppEnabledLayerNames = enabledLayers.data();
            break;
        }
    }

    for (const char* layerName : enabledLayers)
    {
        PHX_CORE_INFO("[Vulkan] - Enabling Vulkan Layer '{0}'", layerName);
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create instance!");

    volkLoadInstance(m_vkInstance);
}

void phx::gfx::platform::VulkanGpuDevice::SetupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = VkDebugCallback;
    createInfo.pUserData = nullptr; // Optional

    VkResult result = CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_debugMessenger);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to set up debug messenger!");
}

void phx::gfx::platform::VulkanGpuDevice::PickPhysicalDevice()
{
    m_vkPhysicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);

    PHX_CORE_INFO("[Vulkan] {0} Physical Devices found", deviceCount);
    if (deviceCount == 0) 
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

    // Use an ordered map to automatically sort candidates by increasing score
    std::multimap<int32_t, VkPhysicalDevice> candidates;

    std::vector<const char*> enabledExtensions;
	for (const auto& device : devices)
	{
        if (!IsDeviceSuitable(device, enabledExtensions))
            continue;

        int32_t score = RateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	// Check if the best candidate is suitable at all
	if (candidates.rbegin()->first > 0)
	{
        m_vkPhysicalDevice = candidates.rbegin()->second;

        IsDeviceSuitable(m_vkPhysicalDevice, enabledExtensions);
        m_extManager.SetEnabledDeviceExtensions(enabledExtensions);
        PHX_CORE_INFO(
            "[Vulkan] Suitable device found {0} - Score:{1}",
            m_properties2.properties.deviceName,
            candidates.rbegin()->first);
	}
	else
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}

    m_extManager.ObtainDeviceExtensions(m_vkPhysicalDevice);

    // Print device info.
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &memProperties);

	PHX_CORE_INFO("[Vulkan] Selected device Memory heaps:");
	// Print memory usage for each heap
	for (uint32_t i = 0; i < memProperties.memoryHeapCount; ++i)
	{
		VkMemoryHeap heap = memProperties.memoryHeaps[i];

		std::string type = {};

		// Step 3: Check heap type based on flags
		if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
		{
			type = "Device Local (GPU memory)";
		}
		else
		{
			type = "Host Visible (CPU accessible)";
		}

		if (heap.flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
		{
			type = "Multi-instance heap";
		}

		PHX_CORE_INFO("\tHeap {0} - {1}: {2} bytes", i, type.c_str(), heap.size);
	}

	PHX_CORE_INFO("[Vulkan] Selected device Memory types:");
	// Print memory usage for each heap
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		VkMemoryType memoryType = memProperties.memoryTypes[i];

		std::stringstream ss = {};
		// Translate property flags into human-readable form
		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
			ss << "Device Local | ";
		}
		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
			ss << "Host Visible | ";
		}
		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
			ss << "Host Coherent | ";
		}
		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
			ss << "Host Cached | ";
		}
		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
			ss << "Lazily Allocated | ";
		}

        std::string properties = ss.str();
        if (properties.empty())
            continue;

		PHX_CORE_INFO("\tHeap {0} - {1}", memoryType.heapIndex, properties.c_str());
	}
}

bool phx::gfx::platform::VulkanGpuDevice::IsDeviceSuitable(VkPhysicalDevice device, std::vector<const char*>& enableExtensions)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    m_extManager.ObtainDeviceExtensions(device);

    m_extManager.ObtainDeviceExtensions(device);

    for (auto req : kRequiredDeviceExtensions)
    {
        if (!m_extManager.IsDeviceExtensionAvailable(req))
        {
            return false;
        }
    }

    bool conservativeRasterization = false;
    m_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    m_vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    m_vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    m_features2.pNext = &m_vulkan11Features;
    m_vulkan11Features.pNext = &m_vulkan12Features;
    m_vulkan12Features.pNext = &m_vulkan13Features;

    void** featuresChain = &m_vulkan13Features.pNext;
    m_accelerationStructureFeatures = {};
    m_raytracingFeatures = {};
    m_raytracingQueryFeatures = {};
    m_fragmentShadingRateFeatures = {};
    m_conditionalRenderingFeatures = {};
    m_depthClipEnableFeatures = {};
    m_meshShaderFeatures = {};
    m_conservativeRasterProperties = {};
    conservativeRasterization = false;

    m_properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    m_properties11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
    m_properties12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
    m_properties13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;

    m_properties2.pNext = &m_properties11;
    m_properties11.pNext = &m_properties12;
    m_properties12.pNext = &m_properties13;

    void** propertiesChain = &m_properties13.pNext;

    m_samplerMinMaxProperties = {};
    m_accelerationStructureProperties = {};
    m_raytracingProperties = {};
    m_fragmentShadingRateProperties = {};
    m_meshShaderProperties = {};


    m_samplerMinMaxProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES;
    *propertiesChain = &m_samplerMinMaxProperties;
    propertiesChain = &m_samplerMinMaxProperties.pNext;

    m_depthStencilResolveProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES;
    *propertiesChain = &m_depthStencilResolveProperties;
    propertiesChain = &m_depthStencilResolveProperties.pNext;

    enableExtensions.clear();
    for (const char* ext : kRequiredDeviceExtensions)
        enableExtensions.push_back(ext);


    if (m_extManager.IsDeviceExtensionAvailable(VK_EXT_IMAGE_VIEW_MIN_LOD_EXTENSION_NAME))
    {
#if false
        enableExtensions.push_back(VK_EXT_IMAGE_VIEW_MIN_LOD_EXTENSION_NAME);
        imageView.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT;
        *featuresChain = &image_view_min_lod_features;
        featuresChain = &image_view_min_lod_features.pNext;
#endif
    }

    if (m_extManager.IsDeviceExtensionAvailable(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME))
    {
        enableExtensions.push_back(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME);

        m_depthClipEnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
        *featuresChain = &m_depthClipEnableFeatures;
        featuresChain = &m_depthClipEnableFeatures.pNext;
    }
    if (m_extManager.IsDeviceExtensionAvailable(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME))
    {
        conservativeRasterization = true;
        enableExtensions.push_back(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);

        m_conservativeRasterProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
        *propertiesChain = &m_conservativeRasterProperties;
        propertiesChain = &m_conservativeRasterProperties.pNext;
    }
    if (m_extManager.IsDeviceExtensionAvailable(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
    {
        enableExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        assert(m_extManager.IsDeviceExtensionAvailable(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME));
        enableExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

        m_accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        *featuresChain = &m_accelerationStructureFeatures;
        featuresChain = &m_accelerationStructureFeatures.pNext;

        m_accelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
        *propertiesChain = &m_accelerationStructureProperties;
        propertiesChain = &m_accelerationStructureProperties.pNext;

        if (m_extManager.IsDeviceExtensionAvailable(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
        {
            enableExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            enableExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);

            m_raytracingQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            *featuresChain = &m_raytracingQueryFeatures;
            featuresChain = &m_raytracingQueryFeatures.pNext;

            m_raytracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
            *propertiesChain = &m_raytracingProperties;
            propertiesChain = &m_raytracingProperties.pNext;
        }

        if (m_extManager.IsDeviceExtensionAvailable(VK_KHR_RAY_QUERY_EXTENSION_NAME))
        {
            enableExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);

            m_raytracingQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
            *featuresChain = &m_raytracingQueryFeatures;
            featuresChain = &m_raytracingQueryFeatures.pNext;
        }
    }

    if (m_extManager.IsDeviceExtensionAvailable(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
    {
        enableExtensions.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
        m_fragmentShadingRateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
        *featuresChain = &m_fragmentShadingRateFeatures;
        featuresChain = &m_fragmentShadingRateFeatures.pNext;

        m_fragmentShadingRateProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
        *propertiesChain = &m_fragmentShadingRateProperties;
        propertiesChain = &m_fragmentShadingRateProperties.pNext;
    }

    if (m_extManager.IsDeviceExtensionAvailable(VK_EXT_MESH_SHADER_EXTENSION_NAME))
    {
        enableExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);

        m_meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        *featuresChain = &m_meshShaderFeatures;
        featuresChain = &m_meshShaderFeatures.pNext;

        m_meshShaderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
        *propertiesChain = &m_meshShaderProperties;
        propertiesChain = &m_meshShaderProperties.pNext;
    }

    if (m_extManager.IsDeviceExtensionAvailable(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME))
    {
        enableExtensions.push_back(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);

        m_conditionalRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
        *featuresChain = &m_conditionalRenderingFeatures;
        featuresChain = &m_conditionalRenderingFeatures.pNext;
    }

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    if (m_extManager.IsDeviceExtensionAvailable(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME) &&
        m_extManager.IsDeviceExtensionAvailable(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME))
    {
        enableExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
        enableExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
    }
#elif defined(__linux__)
    if (m_extManager.IsDeviceExtensionAvailable(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME) &&
        m_extManager.IsDeviceExtensionAvailable(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME))
    {
        enableExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
        enableExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
    }
#endif

    vkGetPhysicalDeviceProperties2(device, &m_properties2);
    return true;
}

void phx::gfx::platform::VulkanGpuDevice::CreateLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies(m_vkPhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = 
    {
        indices.GraphicsFamily.value(),
        indices.ComputeFamily.value(),
        indices.TransferFamily.value(),
    };

    const float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) 
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    assert(m_properties2.properties.limits.timestampComputeAndGraphics == VK_TRUE);

    vkGetPhysicalDeviceFeatures2(m_vkPhysicalDevice, &m_features2);

    assert(m_features2.features.imageCubeArray == VK_TRUE);
    assert(m_features2.features.independentBlend == VK_TRUE);
    assert(m_features2.features.geometryShader == VK_TRUE);
    assert(m_features2.features.samplerAnisotropy == VK_TRUE);
    assert(m_features2.features.shaderClipDistance == VK_TRUE);
    assert(m_features2.features.textureCompressionBC == VK_TRUE);
    assert(m_features2.features.occlusionQueryPrecise == VK_TRUE);
    assert(m_vulkan12Features.descriptorIndexing == VK_TRUE);
    assert(m_vulkan12Features.timelineSemaphore == VK_TRUE);
    assert(m_vulkan13Features.dynamicRendering == VK_TRUE);


#if LOG_DEVICE_EXTENSIONS
    m_extManager.ObtainDeviceExtensions(m_vkPhysicalDevice);
    m_extManager.LogDeviceExtensions();
#endif

    const std::vector<const char*>& enabledExtensions = m_extManager.GetEnabledDeviceExtensions();
    PHX_CORE_INFO("[Vulkan] Enabling Device Extensions: ");
    for (auto ext : enabledExtensions)
    {
        PHX_CORE_INFO("\t\t{0}", ext);
    }

    // Modern Vulkan doesn't require this.
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.enabledLayerCount = 0;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pNext = &m_features2; // Point to the extended features structure
    createInfo.enabledExtensionCount = enabledExtensions.size();
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();

    VkResult result = vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice);
    if (result != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create logical device!");
    }

    volkLoadDevice(m_vkDevice);
    pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(m_vkDevice, "vkSetDebugUtilsObjectNameEXT");
    pfnCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(m_vkDevice, "vkCmdBeginDebugUtilsLabelEXT");
    pfnCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(m_vkDevice, "vkCmdEndDebugUtilsLabelEXT");

    // Query queues
    vkGetDeviceQueue(m_vkDevice, indices.GraphicsFamily.value(), 0, &m_queues[CommandQueueType::Graphics].QueueVk);
    vkGetDeviceQueue(m_vkDevice, indices.ComputeFamily.value(), 0, &m_queues[CommandQueueType::Compute].QueueVk);
    vkGetDeviceQueue(m_vkDevice, indices.TransferFamily.value(), 0, &m_queues[CommandQueueType::Copy].QueueVk);

    m_queueFamilies = indices;

    m_memoryProperties2 = {};
    m_memoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
    vkGetPhysicalDeviceMemoryProperties2(m_vkPhysicalDevice, &m_memoryProperties2);

    if (m_memoryProperties2.memoryProperties.memoryHeapCount == 1 &&
        m_memoryProperties2.memoryProperties.memoryHeaps[0].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
    {
        // https://registry.khronos.org/vulkan/specs/1.0-extensions/html/vkspec.html#memory-device
        //	"In a unified memory architecture (UMA) system there is often only a single memory heap which is
        //	considered to be equally “local” to the host and to the device, and such an implementation must advertise the heap as device-local
        m_capabilities.CacheCoherentUma = true;
    }
}

void phx::gfx::platform::VulkanGpuDevice::CreateSurface(void* windowHandle)
{
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = static_cast<HWND>(windowHandle);
    createInfo.hinstance = GetModuleHandle(nullptr);

    VkResult result = vkCreateWin32SurfaceKHR(m_vkInstance, &createInfo, nullptr, &m_vkSurface);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
}

void phx::gfx::platform::VulkanGpuDevice::CreateSwapchain(SwapChainDesc const& desc)
{
    m_swapChainDesc = desc;
    SwapChainSupportDetails swapChainSupport = QuerySwapchainSupport(m_vkPhysicalDevice);

    VkSurfaceFormatKHR surfaceFormat = {};
    surfaceFormat.format = FormatToVkFormat(desc.Format);
    surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    bool valid = false;
    for (const auto& format : swapChainSupport.Formats)
    {
        if (!desc.EnableHDR && format.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            continue;

        if (format.format == surfaceFormat.format)
        {
            surfaceFormat = format;
            valid = true;
            break;
        }
    }

    if (!valid)
    {
        surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
        surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }

    m_swapChainFormat = surfaceFormat.format;

    if (swapChainSupport.Capabilities.currentExtent.width != 0xFFFFFFFF && swapChainSupport.Capabilities.currentExtent.height != 0xFFFFFFFF)
    {
        m_swapChainExtent = swapChainSupport.Capabilities.currentExtent;
    }
    else
    {
        m_swapChainExtent = { desc.Width, desc.Height };
        m_swapChainExtent.width = std::max(swapChainSupport.Capabilities.minImageExtent.width, std::min(swapChainSupport.Capabilities.maxImageExtent.width, m_swapChainExtent.width));
        m_swapChainExtent.height = std::max(swapChainSupport.Capabilities.minImageExtent.height, std::min(swapChainSupport.Capabilities.maxImageExtent.height, m_swapChainExtent.height));
    }

    uint32_t imageCount = std::max(static_cast<uint32_t>(kBufferCount), swapChainSupport.Capabilities.minImageCount);
    if ((swapChainSupport.Capabilities.maxImageCount > 0) && (imageCount > swapChainSupport.Capabilities.maxImageCount))
    {
        imageCount = swapChainSupport.Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_vkSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // The only one that is always supported
    if (!desc.VSync)
    {
        // The mailbox/immediate present mode is not necessarily supported:
        for (auto& presentMode : swapChainSupport.PresentModes)
        {
            if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }


    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = m_vkSwapChain;

    PHX_CORE_INFO("[Vulkan] - Creating swapchain {0}, {1}", m_swapChainExtent.width, m_swapChainExtent.height);
    VkResult res = vkCreateSwapchainKHR(m_vkDevice, &createInfo, nullptr, &m_vkSwapChain);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to create swapchain");

    // Create Images for back buffers;
    vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imageCount, nullptr);
    m_swapChainImages.resize(static_cast<size_t>(imageCount));
    vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imageCount, m_swapChainImages.data());
}

void phx::gfx::platform::VulkanGpuDevice::RecreateSwapchain()
{
    std::scoped_lock _(m_swapchainMutex);
    WaitForIdle();
    CleanupSwapchain();

    CreateSwapchain(m_swapChainDesc);
    CreateSwapChaimImageViews();

    m_swapchainResized = false;
}

void phx::gfx::platform::VulkanGpuDevice::CleanupSwapchain()
{

    for (auto imageView : m_swapChainImageViews)
    {
        vkDestroyImageView(m_vkDevice, imageView, nullptr);
    }

}

void phx::gfx::platform::VulkanGpuDevice::CreateSwapChaimImageViews()
{
    m_swapChainImageViews.resize(m_swapChainImages.size());
    for (size_t i = 0; i < m_swapChainImages.size(); i++) 
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapChainFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY; 
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(m_vkDevice, &createInfo, nullptr, &m_swapChainImageViews[i]);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void phx::gfx::platform::VulkanGpuDevice::InitializeDescriptorHeaps()
{
    if (m_vulkan12Features.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE)
    {
        m_bindlessSampledImages.Initialize(m_vkDevice, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_properties12.maxDescriptorSetUpdateAfterBindSampledImages / 4);
    }
    if (m_vulkan12Features.descriptorBindingUniformTexelBufferUpdateAfterBind == VK_TRUE)
    {
        m_bindlessUniformTexelBuffers.Initialize(m_vkDevice, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, m_properties12.maxDescriptorSetUpdateAfterBindSampledImages / 4);
    }
    if (m_vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind == VK_TRUE)
    {
        m_bindlessStorageBuffers.Initialize(m_vkDevice, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_properties12.maxDescriptorSetUpdateAfterBindStorageBuffers / 4);
    }
    if (m_vulkan12Features.descriptorBindingStorageImageUpdateAfterBind == VK_TRUE)
    {
        m_bindlessStorageImages.Initialize(m_vkDevice, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_properties12.maxDescriptorSetUpdateAfterBindStorageImages / 4);
    }
    if (m_vulkan12Features.descriptorBindingStorageTexelBufferUpdateAfterBind == VK_TRUE)
    {
        m_bindlessStorageTexelBuffers.Initialize(m_vkDevice, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, m_properties12.maxDescriptorSetUpdateAfterBindStorageImages / 4);
    }
    if (m_vulkan12Features.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE)
    {
        m_bindlessSamplers.Initialize(m_vkDevice, VK_DESCRIPTOR_TYPE_SAMPLER, 256);
    }

    if (m_capabilities.RayTracing)
    {
        m_bindlessAccelerationStructures.Initialize(m_vkDevice, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 32);
    }
}

void phx::gfx::platform::VulkanGpuDevice::CreateVma()
{
    // Initialize Vulkan Memory Allocator helper:
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_vkPhysicalDevice;
    allocatorInfo.device = m_vkDevice;
    allocatorInfo.instance = m_vkInstance;

    // Core in 1.1
    allocatorInfo.flags =
        VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
        VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;

    if (m_vulkan12Features.bufferDeviceAddress)
    {
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    }

#if VMA_DYNAMIC_VULKAN_FUNCTIONS
    static VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;
#endif
    VkResult res = vmaCreateAllocator(&allocatorInfo, &m_vmaAllocator);
    assert(res == VK_SUCCESS);
    if (res != VK_SUCCESS)
    {
        PHX_CORE_ERROR("Failed to create VMA allocator");
        throw std::runtime_error("vmaCreateAllocator failed!");
    }
    
    std::vector<VkExternalMemoryHandleTypeFlags> externalMemoryHandleTypes;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    externalMemoryHandleTypes.resize(m_memoryProperties2.memoryProperties.memoryTypeCount, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);
    allocatorInfo.pTypeExternalMemoryHandleTypes = externalMemoryHandleTypes.data();
#elif defined(__linux__)
    externalMemoryHandleTypes.resize(memory_properties_2.memoryProperties.memoryTypeCount, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
    allocatorInfo.pTypeExternalMemoryHandleTypes = externalMemoryHandleTypes.data();
#endif
    res = vmaCreateAllocator(&allocatorInfo, &m_vmaAllocatorExternal);
    assert(res == VK_SUCCESS);
    if (res != VK_SUCCESS)
    {
        PHX_CORE_ERROR("Failed to create VMA allocator");
        throw std::runtime_error("vmaCreateAllocator failed!");
    }
}

void phx::gfx::platform::VulkanGpuDevice::CreateDefaultResources()
{
    // Create default null descriptors:
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = 4;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.flags = 0;


        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VkResult res = vmaCreateBuffer(m_vmaAllocator, &bufferInfo, &allocInfo, &m_nullBuffer, &m_nullBufferAllocation, nullptr);
        assert(res == VK_SUCCESS);

        VkBufferViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        viewInfo.range = VK_WHOLE_SIZE;
        viewInfo.buffer = m_nullBuffer;
        res = vkCreateBufferView(m_vkDevice, &viewInfo, nullptr, &m_nullBufferView);
        assert(res == VK_SUCCESS);
    }
}

void phx::gfx::platform::VulkanGpuDevice::DestoryDefaultResources()
{
    vmaDestroyBuffer(m_vmaAllocator, m_nullBuffer, m_nullBufferAllocation);
    vkDestroyBufferView(m_vkDevice, m_nullBufferView, nullptr);
}

void phx::gfx::platform::VulkanGpuDevice::CreateFrameResources()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (size_t fr = 0; fr < m_frameFences.size(); ++fr)
    {
        if (vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore[fr]) != VK_SUCCESS ||
            vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore[fr]) != VK_SUCCESS)
            throw std::runtime_error("failed to create synchronization objects for a frame!");

        for (size_t queue = 0; queue < (size_t)CommandQueueType::Count; ++queue)
        {
            if (m_queues[queue].QueueVk == VK_NULL_HANDLE)
                continue;

            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            VkResult res = vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &m_frameFences[fr][queue]);
            assert(res == VK_SUCCESS);
            if (res != VK_SUCCESS)
            {
                PHX_CORE_ERROR("Failed To Create Frame Fence");
                throw std::runtime_error("Failed to create frame fence");
            }
        }
    }
}

void phx::gfx::platform::VulkanGpuDevice::DestroyDescriptorHeaps()
{
    m_bindlessSampledImages.Finalize(m_vkDevice);
    m_bindlessUniformTexelBuffers.Finalize(m_vkDevice);
    m_bindlessStorageBuffers.Finalize(m_vkDevice);
    m_bindlessStorageImages.Finalize(m_vkDevice);
    m_bindlessStorageTexelBuffers.Finalize(m_vkDevice);
    m_bindlessSamplers.Finalize(m_vkDevice);
    m_bindlessAccelerationStructures.Finalize(m_vkDevice);
}

void phx::gfx::platform::VulkanGpuDevice::FinalizeResourcePools()
{
    LOG_AND_FINALIZE_POOL(m_pipelineStatePool);
    LOG_AND_FINALIZE_POOL(m_shaderPool);
    LOG_AND_FINALIZE_POOL(m_bufferPool);
    LOG_AND_FINALIZE_POOL(m_texturePool);
}

void phx::gfx::platform::VulkanGpuDevice::DestoryFrameResources()
{
    for (size_t fr = 0; fr < m_frameFences.size(); ++fr)
    {
        vkDestroySemaphore(m_vkDevice, m_imageAvailableSemaphore[fr], nullptr);
        vkDestroySemaphore(m_vkDevice, m_renderFinishedSemaphore[fr], nullptr);

        for (size_t queue = 0; queue < (size_t)CommandQueueType::Count; ++queue)
        {
            vkDestroyFence(m_vkDevice, m_frameFences[fr][queue], nullptr);
        }
    }
}

int32_t phx::gfx::platform::VulkanGpuDevice::RateDeviceSuitability(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) 
    {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader)
    {
        return 0;
    }

    QueueFamilyIndices indices = FindQueueFamilies(device);

    if (!indices.IsComplete())
    {
        return 0;
    }

    bool swapChainAdequate = false;

    SwapChainSupportDetails swapChainSupport = QuerySwapchainSupport(device);
    if (swapChainSupport.Formats.empty() || swapChainSupport.PresentModes.empty())
    {
        return 0;
    }

    return score;
}

QueueFamilyIndices phx::gfx::platform::VulkanGpuDevice::FindQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = {};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // base queue families
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        auto& queueFamily = queueFamilies[i];

        if (!indices.GraphicsFamily.has_value() && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.GraphicsFamily = i;
        }

        if (!indices.ComputeFamily.has_value() && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            indices.ComputeFamily = i;
        }

        if (!indices.TransferFamily.has_value() && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            indices.TransferFamily = i;
        }
    }


    // Now try to find dedicated compute and transfer queues:
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        auto& queueFamily = queueFamilies[i];

        if (queueFamily.queueCount > 0 &&
            queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            )
        {
            indices.TransferFamily = i;

            if (queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
            {
                // queues[QUEUE_COPY].sparse_binding_supported = true;
            }
        }

        if (queueFamily.queueCount > 0 &&
            queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            )
        {
            indices.ComputeFamily = i;

            if (queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
            {
                // queues[QUEUE_COMPUTE].sparse_binding_supported = true;
            }
        }
    }

#if false
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vkSurface, &presentSupport);
    if (presentSupport)
    {
        indices.PresentFamily = i;
    }
#endif

    return indices;
}

SwapChainSupportDetails phx::gfx::platform::VulkanGpuDevice::QuerySwapchainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_vkSurface, &details.Capabilities);
   
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, nullptr);

    if (formatCount != 0) 
    {
        details.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, details.Formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentModeCount, nullptr);

    if (presentModeCount != 0) 
    {
        details.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentModeCount, details.PresentModes.data());
    }

    return details;
}

void phx::gfx::platform::VulkanGpuDevice::SubmitCommandCtx()
{
    const uint32_t activeCtxCount = m_commandCtxCount;
    m_commandCtxCount = 0;

    for (size_t i = 0; i < (size_t)activeCtxCount; i++)
    {
        CommandCtx_Vulkan* ctx = m_commandCtxPool[i].get();
        VkResult res = vkEndCommandBuffer(ctx->GetVkCommandBuffer());
        assert(res == VK_SUCCESS);

        CommandQueue& queue = m_queues[ctx->QueueType];
        const bool dependency = !ctx->Signals.empty() || !ctx->Waits.empty() || !ctx->WaitQueues.empty();

        if (dependency)
        {
            // NOT SUPPORTED yet
            assert(false);
            PHX_CORE_WARN("Inter Queue Waiting is currently not supported.");

            // If the current commandlist must resolve a dependency, then previous ones will be submitted before doing that:
            //	This improves GPU utilization because not the whole batch of command lists will need to synchronize, but only the one that handles it
            queue.Submit(this, VK_NULL_HANDLE);
        }


        VkCommandBufferSubmitInfo& cbSubmitInfo = queue.SubmitCmds.emplace_back();
        cbSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cbSubmitInfo.commandBuffer = ctx->GetVkCommandBuffer();
    }

    // final submits with fences:
    for (size_t q = 0; q < (size_t)CommandQueueType::Count; q++)
    {
        CommandQueue& queue = m_queues[q];
        if (q == (size_t)CommandQueueType::Graphics)
        {
            VkSemaphoreSubmitInfo& waitSemaphore = queue.SubmitWaitSemaphoreInfos.emplace_back();
            waitSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            waitSemaphore.semaphore = m_imageAvailableSemaphore[GetBufferIndex()];
            waitSemaphore.value = 0; // not a timeline semaphore
            waitSemaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

            queue.SubmitSignalSemaphores.push_back(m_renderFinishedSemaphore[GetBufferIndex()]);
            VkSemaphoreSubmitInfo& signalSemaphore = queue.SubmitSignalSemaphoreInfos.emplace_back();
            signalSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            signalSemaphore.semaphore = m_renderFinishedSemaphore[GetBufferIndex()];
            signalSemaphore.value = 0; // not a timeline semaphore
        }

        queue.Submit(this, m_frameFences[GetBufferIndex()][q]);
    }

}

void phx::gfx::platform::VulkanGpuDevice::Present()
{
    m_dynamicAllocator.EndFrame();

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_renderFinishedSemaphore[GetBufferIndex()];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_vkSwapChain;
    presentInfo.pImageIndices = &m_swapChainCurrentImage;
    VkResult res = vkQueuePresentKHR(m_queues[CommandQueueType::Graphics].QueueVk, &presentInfo);
    if (res != VK_SUCCESS)
    {
        // Handle outdated error in present:
        if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchain();
        }
        else
        {
            assert(0);
        }
    }

    // present
    m_frameCount++;

    // -- Wait on next inflight frame ---
    const size_t backBufferIndex = GetBufferIndex();
    for (size_t q = 0; q < (size_t)CommandQueueType::Count; q++)
    {
        VkFence fence = m_frameFences[backBufferIndex][q];
        vkWaitForFences(m_vkDevice, 1, &fence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_vkDevice, 1, &fence);
    }
}

void phx::gfx::platform::CommandQueue::Submit(VulkanGpuDevice* device, VkFence fence)
{
    if (QueueVk == VK_NULL_HANDLE)
        return;
    std::scoped_lock lock(m_mutex);

    VkSubmitInfo2 submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.commandBufferInfoCount = (uint32_t)SubmitCmds.size();
    submitInfo.pCommandBufferInfos = SubmitCmds.data();

    submitInfo.waitSemaphoreInfoCount = (uint32_t)SubmitWaitSemaphoreInfos.size();
    submitInfo.pWaitSemaphoreInfos = SubmitWaitSemaphoreInfos.data();

    submitInfo.signalSemaphoreInfoCount = (uint32_t)SubmitSignalSemaphoreInfos.size();
    submitInfo.pSignalSemaphoreInfos = SubmitSignalSemaphoreInfos.data();

    VkResult res = vkQueueSubmit2(QueueVk, 1, &submitInfo, fence);
    assert(res == VK_SUCCESS);

    SubmitCmds.clear();
    SubmitWaitSemaphoreInfos.clear();
    SubmitSignalSemaphoreInfos.clear();
}

void phx::gfx::platform::DynamicMemoryAllocator::Initialize(VulkanGpuDevice* device, size_t bufferSize)
{
    m_device = device;
    assert((bufferSize & (bufferSize - 1)) == 0);
    this->m_bufferMask = (bufferSize - 1);

    this->m_buffer = device->CreateBuffer({
            .Usage = Usage::Upload,
            .Binding = BindingFlags::IndexBuffer | BindingFlags::VertexBuffer,
            .SizeInBytes = bufferSize,
            .DebugName = "Temp Buffer"
        });

    m_data = static_cast<uint8_t*>(m_device->GetMappedData(m_buffer));
}

void phx::gfx::platform::DynamicMemoryAllocator::Finalize()
{
    this->m_data = nullptr;

    for (auto& fence : m_fencePool)
    {
        vkDestroyFence(m_device->GetVkDevice(), fence, nullptr);
    }

    this->m_inUseRegions.clear();
    m_device->DeleteBuffer(m_buffer);
}

void phx::gfx::platform::DynamicMemoryAllocator::EndFrame()
{
    // -- Wait on next inflight frame ---
    while (!this->m_inUseRegions.empty())
    {
        auto& region = this->m_inUseRegions.front();
        VkResult result = vkGetFenceStatus(m_device->GetVkDevice(), region.Fence);
        if (result == VK_NOT_READY)
        {
            break;
        }

        vkResetFences(m_device->GetVkDevice(), 1, &region.Fence);
        m_head += region.UsedSize;

        this->m_availableFences.push_back(region.Fence);
        this->m_inUseRegions.pop_front();
    }

    VkFence vkFence = VK_NULL_HANDLE;
    if (!this->m_availableFences.empty())
    {
        vkFence = this->m_availableFences.front();
        this->m_availableFences.pop_front();

    }

    if (vkFence == VK_NULL_HANDLE)
    {
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkResult res = vkCreateFence(m_device->GetVkDevice(), &fenceInfo, nullptr, &vkFence);
        assert(res == VK_SUCCESS);
        m_fencePool.push_back(vkFence);
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkResult result = vkQueueSubmit(m_device->GetVkQueue(CommandQueueType::Graphics), 1, &submitInfo, vkFence);

    this->m_inUseRegions.push_front(
        UsedRegion{
            .UsedSize = m_tail - m_headAtStartOfFrame,
            .Fence = vkFence });

    m_headAtStartOfFrame = m_tail;
}

DynamicMemoryPage phx::gfx::platform::DynamicMemoryAllocator::Allocate(uint32_t allocSize)
{
    std::scoped_lock _(this->m_mutex);

    // Checks if the top bits have changes, if so, we need to wrap around.
    if ((m_tail ^ (m_tail + allocSize)) & ~m_bufferMask)
    {
        m_tail = (m_tail + m_bufferMask) & ~m_bufferMask;
    }

    if (((m_tail - m_head) + allocSize) >= GetBufferSize())
    {
        while (!this->m_inUseRegions.empty())
        {
            auto& region = this->m_inUseRegions.front();
            VkResult result = vkGetFenceStatus(m_device->GetVkDevice(), region.Fence);
            if (result == VK_NOT_READY)
            {
                PHX_CORE_WARN("[GPU QUEUE] Stalling waiting for space");
                vkWaitForFences(m_device->GetVkDevice(), 1, &region.Fence, VK_TRUE, UINT64_MAX);
            }

            vkResetFences(m_device->GetVkDevice(), 1, &region.Fence);
            m_head += region.UsedSize;

            this->m_availableFences.push_back(region.Fence);
            this->m_inUseRegions.pop_front();
        }
    }

    const uint32_t offset = (this->m_tail & m_bufferMask) + allocSize;
    m_tail += allocSize;

    return DynamicMemoryPage{
        .BufferHandle = this->m_buffer,
        .Offset = offset,
        .Data = reinterpret_cast<uint8_t*>(this->m_data + offset),
    };
}

void phx::gfx::platform::BindlessDescriptorHeap::Initialize(VkDevice device, VkDescriptorType type, uint32_t descriptorCount)
{
    descriptorCount = std::min(descriptorCount, 500000u);

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = type;
    poolSize.descriptorCount = descriptorCount;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    VkResult res = vkCreateDescriptorPool(device, &poolInfo, nullptr, &DescriptorPoolVk);
    assert(res == VK_SUCCESS);

    VkDescriptorSetLayoutBinding binding = {};
    binding.descriptorType = type;
    binding.binding = 0;
    binding.descriptorCount = descriptorCount;
    binding.stageFlags = VK_SHADER_STAGE_ALL;
    binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

    VkDescriptorBindingFlags bindingFlags =
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
        VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
        //| VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {};
    bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlagsInfo.bindingCount = 1;
    bindingFlagsInfo.pBindingFlags = &bindingFlags;
    layoutInfo.pNext = &bindingFlagsInfo;

    res = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &DescirptorSetLayoutVk);
    assert(res == VK_SUCCESS);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = DescriptorPoolVk;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &DescirptorSetLayoutVk;
    res = vkAllocateDescriptorSets(device, &allocInfo, &DescritporSetVk);
    assert(res == VK_SUCCESS);

    for (int i = 0; i < (int)descriptorCount; ++i)
    {
        FreeList.push_back((int)descriptorCount - i - 1);
    }
}

void VulkanGpuDevice::CopyCtxManager::Initialize(VulkanGpuDevice* gpuDevice)
{
    m_device = gpuDevice;
}

void VulkanGpuDevice::CopyCtxManager::Finalize()
{
    vkQueueWaitIdle(m_device->GetVkQueue(CommandQueueType::Copy));
    
    for (auto& ctx : FreeList)
    {
        vkDestroyCommandPool(m_device->GetVkDevice(), ctx.TransferCommandPool, nullptr);
        vkDestroyCommandPool(m_device->GetVkDevice(), ctx.TransitionCommandPool, nullptr);
        for (auto& sem : ctx.Semaphores)
            vkDestroySemaphore(m_device->GetVkDevice(), sem, nullptr);

        vkDestroyFence(m_device->GetVkDevice(), ctx.Fence, nullptr);
        m_device->DeleteBuffer(ctx.UploadBuffer);
    }
}

VulkanGpuDevice::CopyCtxManager::Ctx VulkanGpuDevice::CopyCtxManager::Begin(uint64_t stagingSize)
{
    Ctx retVal;
    {
        std::scoped_lock _(m_mutex);

        for (size_t i = 0; i < FreeList.size(); i++)
        {
            if (FreeList[i].UploadBufferSize >= stagingSize)
            {
                if (vkGetFenceStatus(m_device->GetVkDevice(), FreeList[i].Fence) == VK_SUCCESS)
                {
                    retVal = FreeList[i];
                    std::swap(FreeList[i], FreeList.back());
                    FreeList.pop_back();
                    break;
                }
            }
        }
    }

    if (!retVal.IsValid())
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = m_device->m_queueFamilies.TransferFamily.value();
        VkResult res = vkCreateCommandPool(m_device->GetVkDevice(), &poolInfo, nullptr, &retVal.TransferCommandPool);
        poolInfo.queueFamilyIndex = m_device->m_queueFamilies.GraphicsFamily.value();
        res = vkCreateCommandPool(m_device->GetVkDevice(), &poolInfo, nullptr, &retVal.TransitionCommandPool);

        assert(res == VK_SUCCESS);

        VkCommandBufferAllocateInfo commandBufferInfo = {};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferInfo.commandBufferCount = 1;
        commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferInfo.commandPool = retVal.TransferCommandPool;
        res = vkAllocateCommandBuffers(m_device->GetVkDevice(), &commandBufferInfo, &retVal.TransferCommandBuffer);
        assert(res == VK_SUCCESS);

        commandBufferInfo.commandPool = retVal.TransferCommandPool;
        res = vkAllocateCommandBuffers(m_device->GetVkDevice(), &commandBufferInfo, &retVal.TransitionCommandBuffer);
        assert(res == VK_SUCCESS);

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        res = vkCreateFence(m_device->GetVkDevice(), &fenceInfo, nullptr, &retVal.Fence);
        assert(res == VK_SUCCESS);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        res = vkCreateSemaphore(m_device->GetVkDevice(), &semaphoreInfo, nullptr, &retVal.Semaphores[0]);
        assert(res == VK_SUCCESS);

        res = vkCreateSemaphore(m_device->GetVkDevice(), &semaphoreInfo, nullptr, &retVal.Semaphores[1]);
        assert(res == VK_SUCCESS);

        retVal.UploadBufferSize = math::GetNextPowerOfTwo(stagingSize);
        retVal.UploadBufferSize = std::max(retVal.UploadBufferSize, uint64_t(65536));
        retVal.UploadBuffer = m_device->CreateBuffer({
                .Usage = Usage::Upload,
                .SizeInBytes = retVal.UploadBufferSize});

        Buffer_VK* vkBuffer = m_device->m_bufferPool.Get(retVal.UploadBuffer);
        retVal.MappedData = vkBuffer->MappedData;
        
        assert(retVal.UploadBuffer.IsValid());
    }

    // begin command list in valid state:
    VkResult res = vkResetCommandPool(m_device->GetVkDevice(), retVal.TransferCommandPool, 0);
    assert(res == VK_SUCCESS);
    res = vkResetCommandPool(m_device->GetVkDevice(), retVal.TransitionCommandPool, 0);
    assert(res == VK_SUCCESS);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    res = vkBeginCommandBuffer(retVal.TransferCommandBuffer, &beginInfo);
    assert(res == VK_SUCCESS);

    res = vkBeginCommandBuffer(retVal.TransitionCommandBuffer, &beginInfo);
    assert(res == VK_SUCCESS);

    res = vkResetFences(m_device->GetVkDevice(), 1, &retVal.Fence);
    assert(res == VK_SUCCESS);

    return retVal;
}

void VulkanGpuDevice::CopyCtxManager::Submit(Ctx ctx)
{
    VkResult res = vkEndCommandBuffer(ctx.TransferCommandBuffer);
    assert(res == VK_SUCCESS);

    VkSubmitInfo2 submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

    VkCommandBufferSubmitInfo cbSubmitInfo = {};
    cbSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;

    VkSemaphoreSubmitInfo signalSemaphoreInfos[2] = {};
    signalSemaphoreInfos[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfos[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;

    VkSemaphoreSubmitInfo waitSemaphoreInfo = {};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;

    {
        cbSubmitInfo.commandBuffer = ctx.TransferCommandBuffer;
        signalSemaphoreInfos[0].semaphore = ctx.Semaphores[0]; // signal for graphics queue
        signalSemaphoreInfos[0].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &cbSubmitInfo;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = signalSemaphoreInfos;

        std::scoped_lock _(m_device->GetQueue(CommandQueueType::Copy).m_mutex);
        res = vkQueueSubmit2(m_device->GetVkQueue(CommandQueueType::Copy), 1, &submitInfo, VK_NULL_HANDLE);
        assert(res == VK_SUCCESS);
    }

    {
        waitSemaphoreInfo.semaphore = ctx.Semaphores[0]; // wait for copy queue
        waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        cbSubmitInfo.commandBuffer = ctx.TransitionCommandBuffer;
        signalSemaphoreInfos[0].semaphore = ctx.Semaphores[1]; // signal for compute queue
        signalSemaphoreInfos[0].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // signal for compute queue

        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &cbSubmitInfo;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = signalSemaphoreInfos;

        std::scoped_lock _(m_device->GetQueue(CommandQueueType::Graphics).m_mutex);
        res = vkQueueSubmit2(m_device->GetVkQueue(CommandQueueType::Graphics), 1, &submitInfo, VK_NULL_HANDLE);
        assert(res == VK_SUCCESS);
    }


    // This must be final submit in this function because it will also signal a fence for state tracking by CPU!
    {
        waitSemaphoreInfo.semaphore = ctx.Semaphores[1]; // wait for graphics queue
        waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
        submitInfo.commandBufferInfoCount = 0;
        submitInfo.pCommandBufferInfos = nullptr;
        submitInfo.signalSemaphoreInfoCount = 0;
        submitInfo.pSignalSemaphoreInfos = nullptr;

        std::scoped_lock lock(m_device->GetQueue(CommandQueueType::Compute).m_mutex);
        res = vkQueueSubmit2(m_device->GetVkQueue(CommandQueueType::Compute), 1, &submitInfo, ctx.Fence); // final submit also signals fence!
        assert(res == VK_SUCCESS);
    }

    std::scoped_lock _(m_mutex);
    FreeList.push_back(ctx);
}
