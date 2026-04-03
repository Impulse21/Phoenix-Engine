#pragma once

#include <PhxResource/Resource.h>
#include <PhxResource/ResourceTypes.h>
#include <PhxResource/ResourceTypeTraits.h>

#include <PhxRenderer/Shaders/ShaderSystemTypes.h>

#include <slang.h>
#include <slang-com-ptr.h>

namespace phx::renderer
{
    struct ShaderModuleResource final : public Resource
    {
        std::string source_path;
        Slang::ComPtr<slang::IModule> slang_module;
        std::unordered_map<std::string, ShaderStructDesc> types_descs;

        bool FindStruct(const std::string& name, ShaderStructDesc& out_desc);

        void Dispose() override { /* no-op */ };
        bool CollectPendingGpuTransitions(SpanMutable<rhi::GpuBarrier>, size_t&) override { return true; }

        PHX_DECLARE_RESOURCE(ShaderModuleResource)
    };
}


PHX_DEFINE_RESOURCE(
    phx::renderer::ShaderModuleResource,
    ".slang",                       // Extension
    "ShaderModuleLoader"            // Loader ID
);