// PhxRhi/ShaderCompiler.h
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include <PhxCore/Result.h>
#include <PhxRhi/RHICommon.h>

namespace phx
{
    class IVirtualFileSystem;
}

namespace phx::rhi::ShaderCompiler
{

    struct ReflectionField
    {
        std::string name;
        uint32_t    offset;
        uint32_t    size;
    };

    struct PushConstantReflection
    {
        bool is_valid = false;
        uint32_t total_size= 0;
        std::vector<ReflectionField> members;

        uint32_t GetOffset(const std::string& name) const;
    };

    struct BindlessStructReflection
    {
        std::string buffer_name;
        uint32_t stride = 0;
        std::vector<ReflectionField> members;
    };

    struct ShaderReflection
    {
        PushConstantReflection push_constants;
        std::vector<BindlessStructReflection> structured_buffers;
    };

    struct Input
    {
        // Common settings
        union
        {
            struct
            {
                uint8_t disable_optimizations : 1;
                uint8_t generate_debug_info : 1;
                uint8_t strip_reflection : 1;
                uint8_t warnings_as_errors : 1;
                uint8_t padding : 4;
            };
            uint8_t flags = 0;
        };

        ShaderFormat format = ShaderFormat::None;
        ShaderModel shader_model = ShaderModel::SM_6_6; // Or generic profile

        std::string source_filename;
        Span<const char*> include_dir;
        Span<const char*> defines;

        std::string vs_entry_point = "VS_Main";
        std::string ps_entry_point = "PS_Main";
        std::string cs_entry_point = "CS_Main";
    };

    // --- 3. The Compiler Output ---

    struct Output
    {
        std::vector<uint8_t> byte_code;
        ShaderReflection reflection;
        std::vector<std::string> dependencies;
    };

    // The single entry point
    phx::Result<Output, std::string> Compile(IVirtualFileSystem* vfs, Input const& input);
}