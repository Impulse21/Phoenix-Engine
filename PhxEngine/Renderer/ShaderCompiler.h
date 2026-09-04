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

    // Default layout for constant/push-constant buffer matrix fields that
    // don't specify row_major/column_major explicitly in the shader source.
    enum class MatrixLayout : u8
    {
        RowMajor,
        ColumnMajor,
    };

    enum class OptimizationLevel : u8
    {
        None,     // Don't optimize at all.
        Default,  // Balance code quality and compile time.
        High,     // Optimize aggressively.
        Maximal,  // May take a very long time.
    };

    enum class DebugInfoLevel : u8
    {
        None,     // Don't emit debug information at all.
        Minimal,  // As little as possible — just enough for stack traces.
        Standard, // Whatever is the standard level for the target.
        Maximal,  // As much as possible, potentially disabling optimizations.
    };

    struct InitParams
    {
        // RowMajor matches hlslpp's native in-memory float4x4 layout, so
        // CPU-side matrices can be pushed as-is with no per-upload
        // transpose. (HLSL/D3D's own default is ColumnMajor — pass that
        // explicitly if authoring against hlslpp isn't the goal.)
        MatrixLayout      matrix_layout = MatrixLayout::RowMajor;
        OptimizationLevel optimization  = OptimizationLevel::High;
        DebugInfoLevel    debug_info    = DebugInfoLevel::Standard;
    };

    bool Initialize(const InitParams& params = {});
    void Shutdown();

    // Compiles the entry point of a .slang source file (loaded via the VFS)
    // to SPIR-V. Returns an error on failure — diagnostics are logged.
    [[nodiscard]] Result<MemoryBuffer> Compile(const char* virtual_path, const char* entry_point, Stage stage);
}
