#include "HordeRenderer.h"

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/Log.h>

#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/Renderer/ShaderCompiler.h>


using namespace horde;
using namespace phx;
using namespace phx::rhi;

namespace
{
    constexpr Log::Channel k_log = { "Hord Renderer" };
	constexpr u64 k_buffer_heap_size = 8_MB;
    constexpr u64 k_texture_heap_size = 16_MB;
}

bool horde::HordeRenderer::Initialize() noexcept
{
    // TODO: Allocate memory for GPU
    // TODO: init render targets
    // TODO: init samplers

    // Create PSOs and load shaders

    rhi::DeviceCapabilities cap = rhi::GetDeviceCapabilities();
    PHX_ASSERT(EnumHasAnyFlags(cap.features, rhi::DeviceFeatures::MeshShaders));


    rhi::ViewportDesc present_desc;
    if (!rhi::GetViewportDesc(present_desc))
    {
        PHX_LOG_ERROR(k_log, "Initialize failed — RHI has no viewport yet");
        return false;
    }

    ShaderCompiler::Initialize();

    {
        auto primitive_module_spriv = ShaderCompiler::CompileModule("shaders://primitives.slang");
        if (!primitive_module_spriv)
        {
            PHX_LOG_ERROR(k_log, "Failed to compile primitive codes");
            ShaderCompiler::Shutdown();
            return false;
        }

        rhi::ShaderModuleHandle primitive_shader_module = rhi::CreateShaderModule({
            .byte_code = Span<u32>(
                reinterpret_cast<const u32*>(primitive_module_spriv->Data()),
                primitive_module_spriv->Size() / sizeof(u32)),
        });

        m_pso[Pso::Box] =
            CreatePso({
                { .stage = rhi::ShaderStage::MS, .module_handle = primitive_shader_module, .entry_point = "MS_Box" },
                { .stage = rhi::ShaderStage::FS, .module_handle = primitive_shader_module, .entry_point = "FS_Main" }
        },
        present_desc);

        m_pso[Pso::Capsule] = 
            CreatePso({
                { .stage = rhi::ShaderStage::MS, .module_handle = primitive_shader_module, .entry_point = "MS_Capsule" },
                { .stage = rhi::ShaderStage::FS, .module_handle = primitive_shader_module, .entry_point = "FS_Main" }
        },
        present_desc);

        m_pso[Pso::Plane] = 
            CreatePso({
                { .stage = rhi::ShaderStage::MS, .module_handle = primitive_shader_module, .entry_point = "MS_Plane" },
                { .stage = rhi::ShaderStage::FS, .module_handle = primitive_shader_module, .entry_point = "FS_Main" }
        },
        present_desc);

        rhi::DestroyShaderModule(primitive_shader_module);
    }

    ShaderCompiler::Shutdown();
    return true;
}

void horde::HordeRenderer::Shutdown()
{
    for (rhi::PipelineStateHandle& pso : m_pso)
        rhi::DestroyPipelineState(pso);
}

phx::rhi::PipelineStateHandle horde::HordeRenderer::CreatePso(phx::Span<phx::rhi::ShaderStageInfo> shader_stages, const phx::rhi::ViewportDesc& viewport_desc) const
{
    rhi::Format colour_format = viewport_desc.format;
    return rhi::CreatePipelineState({
            .type           = rhi::PipelineType::Graphics,
            .shader_stages  = shader_stages,
            .depth_stencil_state = {
                .depth_enable     = true,
                .depth_write_mask = rhi::DepthWriteMask::All,
                .depth_func       = rhi::ComparisonFunc::Less, // matches the depth_clear = 1.0f (far) convention used in OnRender
            },
            .raster_state = {
                .cull_mode = rhi::RasterCullMode::Back,
                .front_counter_clockwise = !rhi::IsClipSpaceYDown(),
            },
            .prim_type      = rhi::PrimitiveType::TriangleList,
            .render_pass_info = {
                .color_attachments = Span<rhi::Format>(&colour_format, 1),
                .depth_stencil_format = viewport_desc.depth_format,
            },
        });
}
