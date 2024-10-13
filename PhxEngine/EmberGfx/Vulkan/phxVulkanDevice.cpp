#include "pch.h"

#include "phxVulkanCore.h"
#include "phxVulkanDevice.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "vma/vk_mem_alloc.h"

#include "vulkan/vulkan_win32.h"
#include "spriv-reflect/spirv_reflect.h"

#include <set>
#include <map>

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
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
    };

    // Optional extensions can also be done in the same way
    const std::vector<const char*> kOptionalExtensions =
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
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
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
        VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,

        "VK_GOOGLE_hlsl_functionality1",
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

    CreateVma();
    CreateFrameResources();
    CreateDefaultResources();
}

void phx::gfx::platform::VulkanGpuDevice::Finalize()
{
    this->RunGarbageCollection();

    DestoryFrameResources();
    DestoryDefaultResources();

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


    for (auto imageView : m_swapChainImageViews) 
    {
        vkDestroyImageView(m_vkDevice, imageView, nullptr);
    }

    vmaDestroyAllocator(m_vmaAllocator);
    vkDestroyPipelineCache(m_vkDevice, m_vkPipelineCache, nullptr);
    vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);
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

CommandCtx_Vulkan* phx::gfx::platform::VulkanGpuDevice::BeginCommandCtx(phx::gfx::CommandQueueType type)
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
            }
        }
    };
}

PipelineStateHandle phx::gfx::platform::VulkanGpuDevice::CreatePipeline(PipelineStateDesc2 const& desc)
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
            }
        }
    };
}

void phx::gfx::platform::VulkanGpuDevice::CreateInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "PHX Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

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

	for (const auto& device : devices)
	{
        int32_t score = RateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	// Check if the best candidate is suitable at all
	if (candidates.rbegin()->first > 0)
	{
        m_vkPhysicalDevice = candidates.rbegin()->second;

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &deviceProperties);
        PHX_CORE_INFO(
            "[Vulkan] Suitable device found {0} - Score:{1}",
            deviceProperties.deviceName,
            candidates.rbegin()->first);
	}
	else
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}

    m_extManager.ObtainDeviceExtensions(m_vkPhysicalDevice);
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

    VkPhysicalDeviceFeatures deviceFeatures{};

  
    m_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    m_features2.pNext = &m_vulkan12Features;

    // Optionally, query other feature structures you might need
    m_vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    m_vulkan12Features.pNext = &m_vulkan13Features;

    vkGetPhysicalDeviceFeatures2(m_vkPhysicalDevice, &m_features2);

    // -- enable dynamic rendering ---
    m_vulkan13Features.dynamicRendering = true;

    // -- Enable extended Dynamic rendering ---
    m_extendedDynamicStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    m_extendedDynamicStateFeatures.extendedDynamicState = VK_TRUE; // Enable extended dynamic state
    m_vulkan13Features.pNext = &m_extendedDynamicStateFeatures;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.enabledExtensionCount = kRequiredDeviceExtensions.size();
    createInfo.ppEnabledExtensionNames = kRequiredDeviceExtensions.data();

    // Modern Vulkan doesn't require this.
    createInfo.enabledLayerCount = 0;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pNext = &m_features2; // Point to the extended features structure

    VkResult result = vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice);
    if (result != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create logical device!");
    }

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
        //	considered to be equally �local� to the host and to the device, and such an implementation must advertise the heap as device-local
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
    res = vmaCreateAllocator(&allocatorInfo, &m_vmaAllocator);
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
    for (size_t fr = 0; fr < m_frameFences.size(); ++fr)
    {
        for (size_t queue = 0; queue < (size_t)CommandQueueType::Count; ++queue)
        {
            if (m_queues[queue].QueueVk == VK_NULL_HANDLE)
                continue;

            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            //fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
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

void phx::gfx::platform::VulkanGpuDevice::DestoryFrameResources()
{
    for (size_t fr = 0; fr < m_frameFences.size(); ++fr)
    {
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

    m_extManager.ObtainDeviceExtensions(device);

    for (auto req : kRequiredDeviceExtensions)
    {
        if (!m_extManager.IsDeviceExtensionAvailable(req))
        {
            return score = 0;
        }
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
            // If the current commandlist must resolve a dependency, then previous ones will be submitted before doing that:
            //	This improves GPU utilization because not the whole batch of command lists will need to synchronize, but only the one that handles it
            queue.Submit(this, VK_NULL_HANDLE);
        }


        VkCommandBufferSubmitInfo& cbSubmitInfo = queue.SubmitCmds.emplace_back();
        cbSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        cbSubmitInfo.commandBuffer = ctx->GetVkCommandBuffer();
    }

    // final submits with fences:
    for (size_t q = 0; qauto& queue : m_queues)
    {
        queue.Submit(this, m_frameFences[GetBufferIndex(), ]);
        queues[q].submit(this, frame_fence[GetBufferIndex()][q]);
    }

}

void phx::gfx::platform::VulkanGpuDevice::CommandQueue::Signal(VkSemaphore semaphore)
{
}

void phx::gfx::platform::VulkanGpuDevice::CommandQueue::Wait(VkSemaphore semaphore)
{
}

void phx::gfx::platform::VulkanGpuDevice::CommandQueue::Submit(VulkanGpuDevice* device, VkFence fence)
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