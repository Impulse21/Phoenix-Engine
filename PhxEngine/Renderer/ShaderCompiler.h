#pragma once

#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/Result.h>

// TODO
// Consider making this an interface or opaueu class
// so that we use depedency injection and support
// compiled vs non compiled shader sources.
namespace phx::ShaderCompiler
{
    enum class Stage : u8
    {
        Vertex,
        Fragment,
        Compute,
        Task,
        Mesh,
    };

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

    struct ShaderTypeArg
    {
        const char* module_name;
        const char* type_name;
    };

    struct ShaderVariant
    {
        phx::Span<ShaderTypeArg> type_args;
        [[nodiscard]] bool HasArgs() const { return !type_args.IsEmpty(); }
        [[nodiscard]] u64 Hash() const { return 0ul; }
    };

    struct InitParams
    {
        MatrixLayout      matrix_layout = MatrixLayout::RowMajor;
        OptimizationLevel optimization  = OptimizationLevel::High;
        DebugInfoLevel    debug_info    = DebugInfoLevel::Standard;

        phx::Span<const char*> shader_search_paths;
    };

    bool Initialize(const InitParams& params = {});
    void Shutdown();

    [[nodiscard]] Result<MemoryBuffer> Compile(const char* virtual_path, const char* entry_point, Stage stage, const ShaderVariant& variant = {});
    [[nodiscard]] Result<MemoryBuffer> Compile(const MemoryBuffer& source, const char* virtual_path, const char* entry_point, Stage stage, const ShaderVariant& variant = {});

    // Compiles every shader.
    [[nodiscard]] Result<MemoryBuffer> CompileModule(const char* virtual_path);
    [[nodiscard]] Result<MemoryBuffer> CompileModule(const MemoryBuffer& source, const char* virtual_path);
}
