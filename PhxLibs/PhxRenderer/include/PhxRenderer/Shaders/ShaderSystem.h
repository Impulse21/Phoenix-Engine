#pragma once

#include <PhxCore/Handle.h>
#include <PhxCore/Span.h>

#include <string>

namespace phx::renderer
{
    struct Shader;
    using ShaderHandle = Handle<Shader>;

    namespace ShaderSystem
    {
        void Initialize(Span<std::string> preload_list);
        void Shutdown();

        void RegisterModule(const std::string& name, const std::string& path);
        void RegisterModule(const std::string& name, const std::string& path, const void* data, size_t data_size);

        ShaderHandle RequestShaderProgram(std::string virtual_path);
    }
}