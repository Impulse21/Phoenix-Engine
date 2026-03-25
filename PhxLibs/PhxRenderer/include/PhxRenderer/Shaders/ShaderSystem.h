#pragma once

#include <PhxCore/Handle.h>
#include <PhxRenderer/Shaders/ShaderSystemTypes.h>

#include <string>

namespace phx::renderer
{
    struct Shader;
    using ShaderHandle = Handle<Shader>;

    namespace ShaderSystem
    {
        void Initialize(Span<std::string> preload_list);
        void Shutdown();

        void RegisterModule(const std::string& virtual_path);
        void RegisterModule(const std::string& name, const std::string& virtual_path, const void* data, size_t data_size);

        ShaderDescriptor MakePassDescriptor(const ShaderDescriptor& base_desc, Span<PassInfo> pass_info);
        ShaderHandle GetOrRequestProgram(const ShaderDescriptor& shader_desc);
    }
}