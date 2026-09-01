#pragma once

#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/Core/Result.h>

namespace phx::ShaderCompiler
{
    enum class Stage : u8
    {
        Vertex,
        Fragment,
        Compute,
    };

    bool Initialize();
    void Shutdown();

    // Compiles the entry point of a .slang source file (loaded via the VFS)
    // to SPIR-V. Returns an error on failure — diagnostics are logged.
    [[nodiscard]] Result<MemoryBuffer> Compile(const char* virtual_path, const char* entry_point, Stage stage);
}
