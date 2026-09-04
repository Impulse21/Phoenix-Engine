#include "ShaderCompiler.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/VFS/VFS.h>

#include <slang-com-ptr.h>
#include <slang.h>

#include <cstring>
#include <string>

using namespace phx;

namespace
{
    constexpr Log::Channel k_log = { "ShaderCompiler" };

    Slang::ComPtr<slang::IGlobalSession> s_global_session;
    Slang::ComPtr<slang::ISession>       s_session;

    SlangStage ToSlangStage(ShaderCompiler::Stage stage)
    {
        switch (stage)
        {
            case ShaderCompiler::Stage::Vertex:
                return SLANG_STAGE_VERTEX;

            case ShaderCompiler::Stage::Fragment:
                return SLANG_STAGE_FRAGMENT;

            case ShaderCompiler::Stage::Compute:
                return SLANG_STAGE_COMPUTE;
        }

        return SLANG_STAGE_NONE;
    }

    SlangMatrixLayoutMode ToSlangMatrixLayout(ShaderCompiler::MatrixLayout layout)
    {
        switch (layout)
        {
            case ShaderCompiler::MatrixLayout::RowMajor:
                return SLANG_MATRIX_LAYOUT_ROW_MAJOR;

            case ShaderCompiler::MatrixLayout::ColumnMajor:
                return SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
        }

        return SLANG_MATRIX_LAYOUT_MODE_UNKNOWN;
    }

    SlangOptimizationLevel ToSlangOptimizationLevel(ShaderCompiler::OptimizationLevel level)
    {
        switch (level)
        {
            case ShaderCompiler::OptimizationLevel::None:
                return SLANG_OPTIMIZATION_LEVEL_NONE;

            case ShaderCompiler::OptimizationLevel::Default:
                return SLANG_OPTIMIZATION_LEVEL_DEFAULT;

            case ShaderCompiler::OptimizationLevel::High:
                return SLANG_OPTIMIZATION_LEVEL_HIGH;

            case ShaderCompiler::OptimizationLevel::Maximal:
                return SLANG_OPTIMIZATION_LEVEL_MAXIMAL;
        }

        return SLANG_OPTIMIZATION_LEVEL_DEFAULT;
    }

    SlangDebugInfoLevel ToSlangDebugInfoLevel(ShaderCompiler::DebugInfoLevel level)
    {
        switch (level)
        {
            case ShaderCompiler::DebugInfoLevel::None:
                return SLANG_DEBUG_INFO_LEVEL_NONE;

            case ShaderCompiler::DebugInfoLevel::Minimal:
                return SLANG_DEBUG_INFO_LEVEL_MINIMAL;

            case ShaderCompiler::DebugInfoLevel::Standard:
                return SLANG_DEBUG_INFO_LEVEL_STANDARD;

            case ShaderCompiler::DebugInfoLevel::Maximal:
                return SLANG_DEBUG_INFO_LEVEL_MAXIMAL;
        }

        return SLANG_DEBUG_INFO_LEVEL_STANDARD;
    }

    // Diagnostics blobs are null-terminated human-readable text on success or failure alike.
    void LogDiagnostics(slang::IBlob* diagnostics)
    {
        if (diagnostics && diagnostics->getBufferSize() > 0)
            PHX_LOG_WARN(k_log, "{0}", (const char*)diagnostics->getBufferPointer());
    }
}

bool phx::ShaderCompiler::Initialize(const InitParams& params)
{
    if (SLANG_FAILED(slang::createGlobalSession(s_global_session.writeRef())))
    {
        PHX_LOG_ERROR(k_log, "Failed to create Slang global session");
        return false;
    }

    slang::CompilerOptionEntry options[] = {
        { slang::CompilerOptionName::EmitSpirvDirectly,         { slang::CompilerOptionValueKind::Int, 1 } },
        { slang::CompilerOptionName::VulkanUseEntryPointName,   { slang::CompilerOptionValueKind::Int, 1 } },
        { slang::CompilerOptionName::Optimization,              { slang::CompilerOptionValueKind::Int, static_cast<int32_t>(ToSlangOptimizationLevel(params.optimization)) } },
        { slang::CompilerOptionName::DebugInformation,          { slang::CompilerOptionValueKind::Int, static_cast<int32_t>(ToSlangDebugInfoLevel(params.debug_info)) } },
    };

    slang::TargetDesc target       = {
        .format                  = SLANG_SPIRV,
        .profile                 = s_global_session->findProfile("spirv_1_6"),
        .compilerOptionEntries   = options,
        .compilerOptionEntryCount = PHX_ARRAY_COUNT(options),
    };

    slang::SessionDesc session_desc      = {
        .targets                 = &target,
        .targetCount             = 1,
        .defaultMatrixLayoutMode = ToSlangMatrixLayout(params.matrix_layout),
    };

    if (SLANG_FAILED(s_global_session->createSession(session_desc, s_session.writeRef())))
    {
        PHX_LOG_ERROR(k_log, "Failed to create Slang session");
        s_global_session = nullptr;

        return false;
    }

    PHX_LOG_INFO(k_log, "Initialized — targeting SPIR-V");

    return true;
}

void phx::ShaderCompiler::Shutdown()
{
    s_session        = nullptr;
    s_global_session = nullptr;

    PHX_LOG_INFO(k_log, "Shutdown complete");
}

Result<MemoryBuffer> phx::ShaderCompiler::Compile(const char* virtual_path, const char* entry_point, Stage stage)
{
    MemoryBuffer source = VFS::ReadFile(virtual_path);
    if (source.IsEmpty())
    {
        PHX_LOG_ERROR(k_log, "Could not read shader source '{}'", virtual_path);
        return Unexpected(ResultError::NotFound);
    }

    // loadModuleFromSourceString wants a null-terminated string; the VFS
    // buffer is raw file bytes with no such guarantee, so stage it through
    // a std::string rather than pass the buffer pointer directly.
    // Is this allocation really needed?
    std::string source_text((const char*)source.Data(), source.Size());

    Slang::ComPtr<slang::IBlob> diagnostics;

    slang::IModule* module = 
        s_session->loadModuleFromSourceString(
            virtual_path,
            virtual_path,
            source_text.c_str(),
            diagnostics.writeRef());

    LogDiagnostics(diagnostics);

    if (!module)
    {
        PHX_LOG_ERROR(k_log, "Failed to compile module '{0}'", virtual_path);
        return Unexpected(ResultError::Failure);
    }

    Slang::ComPtr<slang::IEntryPoint> entry;
    if (SLANG_FAILED(module->findAndCheckEntryPoint(entry_point, ToSlangStage(stage), entry.writeRef(), diagnostics.writeRef())))
    {
        LogDiagnostics(diagnostics);
        PHX_LOG_ERROR(k_log, "Entry point '{0}' not found in '{1}'", entry_point, virtual_path);
        return Unexpected(ResultError::NotFound);
    }

    slang::IComponentType* components[] = { module, entry };

    Slang::ComPtr<slang::IComponentType> program;
    if (SLANG_FAILED(s_session->createCompositeComponentType(components, PHX_ARRAY_COUNT(components), program.writeRef(), diagnostics.writeRef())))
    {
        LogDiagnostics(diagnostics);
        PHX_LOG_ERROR(k_log, "Failed to link '{0}'", virtual_path);
        return Unexpected(ResultError::Failure);
    }

    Slang::ComPtr<slang::IBlob> code;
    if (SLANG_FAILED(program->getEntryPointCode(0, 0, code.writeRef(), diagnostics.writeRef())))
    {
        LogDiagnostics(diagnostics);
        PHX_LOG_ERROR(k_log, "Code generation failed for '{0}'", virtual_path);
        return Unexpected(ResultError::Failure);
    }

    const usize size = code->getBufferSize();
    MemoryBuffer spirv(size);
    memcpy(spirv.Data(), code->getBufferPointer(), size);

    return spirv;
}
