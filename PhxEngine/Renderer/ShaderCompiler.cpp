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
            case ShaderCompiler::Stage::Vertex:   return SLANG_STAGE_VERTEX;
            case ShaderCompiler::Stage::Fragment: return SLANG_STAGE_FRAGMENT;
            case ShaderCompiler::Stage::Compute:  return SLANG_STAGE_COMPUTE;
        }
        return SLANG_STAGE_NONE;
    }

    // Diagnostics blobs are null-terminated human-readable text on success or failure alike.
    void LogDiagnostics(slang::IBlob* diagnostics)
    {
        if (diagnostics && diagnostics->getBufferSize() > 0)
            PHX_LOG_WARN(k_log, "{}", (const char*)diagnostics->getBufferPointer());
    }
}

bool phx::ShaderCompiler::Initialize()
{
    if (SLANG_FAILED(slang::createGlobalSession(s_global_session.writeRef())))
    {
        PHX_LOG_ERROR(k_log, "Failed to create Slang global session");
        return false;
    }

    // Mirrors the flags SlangShader.cmake passes to slangc for build-time
    // shader compilation, so runtime and build-time output are equivalent.
    slang::CompilerOptionEntry options[] = {
        { slang::CompilerOptionName::EmitSpirvDirectly,       { slang::CompilerOptionValueKind::Int, 1 } },
        { slang::CompilerOptionName::VulkanUseEntryPointName, { slang::CompilerOptionValueKind::Int, 1 } },
    };

    slang::TargetDesc target       = {};
    target.format                  = SLANG_SPIRV;
    target.profile                 = s_global_session->findProfile("spirv_1_5");
    target.compilerOptionEntries   = options;
    target.compilerOptionEntryCount = PHX_ARRAY_COUNT(options);

    slang::SessionDesc session_desc      = {};
    session_desc.targets                 = &target;
    session_desc.targetCount             = 1;
    session_desc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

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
    std::string source_text((const char*)source.Data(), source.Size());

    Slang::ComPtr<slang::IBlob> diagnostics;

    // Owned by the session — not our pointer to release.
    slang::IModule* module = s_session->loadModuleFromSourceString(
        virtual_path, virtual_path, source_text.c_str(), diagnostics.writeRef());

    LogDiagnostics(diagnostics);

    if (!module)
    {
        PHX_LOG_ERROR(k_log, "Failed to compile module '{}'", virtual_path);
        return Unexpected(ResultError::Failure);
    }

    Slang::ComPtr<slang::IEntryPoint> entry;
    if (SLANG_FAILED(module->findAndCheckEntryPoint(entry_point, ToSlangStage(stage), entry.writeRef(), diagnostics.writeRef())))
    {
        LogDiagnostics(diagnostics);
        PHX_LOG_ERROR(k_log, "Entry point '{}' not found in '{}'", entry_point, virtual_path);
        return Unexpected(ResultError::NotFound);
    }

    slang::IComponentType* components[] = { module, entry };

    Slang::ComPtr<slang::IComponentType> program;
    if (SLANG_FAILED(s_session->createCompositeComponentType(components, PHX_ARRAY_COUNT(components), program.writeRef(), diagnostics.writeRef())))
    {
        LogDiagnostics(diagnostics);
        PHX_LOG_ERROR(k_log, "Failed to link '{}'", virtual_path);
        return Unexpected(ResultError::Failure);
    }

    Slang::ComPtr<slang::IBlob> code;
    if (SLANG_FAILED(program->getEntryPointCode(0, 0, code.writeRef(), diagnostics.writeRef())))
    {
        LogDiagnostics(diagnostics);
        PHX_LOG_ERROR(k_log, "Code generation failed for '{}'", virtual_path);
        return Unexpected(ResultError::Failure);
    }

    const usize size = code->getBufferSize();
    MemoryBuffer spirv(size);
    memcpy(spirv.Data(), code->getBufferPointer(), size);

    return spirv;
}
