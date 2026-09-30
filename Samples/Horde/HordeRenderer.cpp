#include "HordeRenderer.h"

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/Log.h>

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

bool horde::HordeRenderer::Initialize()
{
    // TODO: Allocate memory for GPU
    // TODO: init render targets
    // TODO: init samplers

    // Create PSOs and load shaders

    rhi::DeviceCapabilities cap = rhi::GetDeviceCapabilities();
    PHX_ASSERT(EnumHasAnyFlags(cap.features, rhi::DeviceFeatures::MeshShaders));

    {
        auto ms_result = ShaderCompiler::Compile("shaders://Cube.slang", "MS_Main", ShaderCompiler::Stage::Mesh);

        if (!ms_result)
        {
            PHX_LOG_ERROR(k_log, "Failed to compile Cube.slang");
            return false;
        }


        auto ms_shader_module = rhi::CreateShaderModule({
            .byte_code = Span<u32>(
                reinterpret_cast<const u32*>(ms_result->Data()),
                ms_result->Size() / sizeof(u32)),
        });

        rhi::ShaderStageInfo stages[] = {
            { .stage = rhi::ShaderStage::MS, .module_handle = ms_shader_module, .entry_point = "MS_Main" },
        };

        m_pso[Pso::Box] = rhi::CreatePipelineState({
        });
        rhi::DestroyShaderModule(ms_shader_module);
    }

}
