#pragma once

#include <PhxCore/Handle.h>
#include <PhxRenderer/Shaders/ShaderSystemTypes.h>

#include <string>

namespace phx::renderer
{
    struct Shader;
    using ShaderHandle = Handle<Shader>;

    struct ShaderModule;
    using ShaderModuleHandle = Handle<ShaderModule>;

    namespace ShaderSystem
    {
        void Initialize(Span<std::string> preload_list, uint32_t max_shader_modules);
        void Shutdown();

        bool RegisterModule(const std::string& virtual_path);
        bool RegisterModule(const std::string& name, const std::string& virtual_path, const void* data, size_t data_size);

        ShaderDescriptor MakePassDescriptor(const ShaderDescriptor& base_desc, Span<PassInfo> pass_info);
        ShaderHandle GetOrRequestProgram(const ShaderDescriptor& shader_desc);

        // This is a leak in API, but for now it's okay as I don't have
        // time to implement an abstraction
        bool FindStruct(
            const std::string& virtual_path,
            std::string_view struct_name,
            ShaderStructDesc& out_desc);
    }
}