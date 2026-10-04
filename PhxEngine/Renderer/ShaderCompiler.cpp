#include "ShaderCompiler.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/VFS/VFS.h>

#include <slang-com-ptr.h>
#include <slang.h>

#include <cstring>
#include <vector>

using namespace phx;

namespace
{
    constexpr Log::Channel k_log = { "ShaderCompiler" };

    Slang::ComPtr<slang::IGlobalSession> s_global_session;
    Slang::ComPtr<slang::ISession>       s_session;

    bool IsGeneric(slang::IEntryPoint* entry_point)
    {
        return entry_point->getFunctionReflection()->getGenericContainer() != nullptr;
    }

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

            case ShaderCompiler::Stage::Task:
                return SLANG_STAGE_AMPLIFICATION;

            case ShaderCompiler::Stage::Mesh:
                return SLANG_STAGE_MESH;
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

    slang::IModule* LoadModule(const MemoryBuffer& source, const char* virtual_path)
    {
        Slang::ComPtr<slang::IBlob> source_blob;
        source_blob.attach(slang_createBlob(source.Data(), source.Size()));

        Slang::ComPtr<slang::IBlob> diagnostics;
        slang::IModule* module =
            s_session->loadModuleFromSource(
                virtual_path,
                virtual_path,
                source_blob,
                diagnostics.writeRef());

        LogDiagnostics(diagnostics);

        if (!module)
            PHX_LOG_ERROR(k_log, "Failed to compile module '{0}'", virtual_path);

        return module;
    }

    class VfsSlangFileSystem final : public ISlangFileSystem
    {
    public:
        SLANG_NO_THROW SlangResult SLANG_MCALL queryInterface(SlangUUID const& uuid, void** outObject) override
        {
            const SlangUUID candidates[] = { ISlangFileSystem::getTypeGuid(), ISlangCastable::getTypeGuid(), ISlangUnknown::getTypeGuid() };
            for (const SlangUUID& candidate : candidates)
            {
                if (std::memcmp(&uuid, &candidate, sizeof(SlangUUID)) == 0)
                {
                    *outObject = static_cast<ISlangFileSystem*>(this);
                    return SLANG_OK;
                }
            }

            *outObject = nullptr;
            return SLANG_E_NO_INTERFACE;
        }

        SLANG_NO_THROW uint32_t SLANG_MCALL addRef() override { return 1; }
        SLANG_NO_THROW uint32_t SLANG_MCALL release() override { return 1; }
        SLANG_NO_THROW void* SLANG_MCALL castAs(const SlangUUID&) override { return nullptr; }

        SLANG_NO_THROW SlangResult SLANG_MCALL loadFile(char const* path, ISlangBlob** outBlob) override
        {
            MemoryBuffer bytes = VFS::ReadFile(path);
            if (bytes.IsEmpty())
                return SLANG_E_CANNOT_OPEN;

            PHX_LOG_INFO(k_log, "Loaded module '{0}'", path);
            *outBlob = slang_createBlob(bytes.Data(), bytes.Size());
            return SLANG_OK;
        }
    };

    VfsSlangFileSystem s_vfs_file_system;
}

bool phx::ShaderCompiler::Initialize(const InitParams& params)
{
    if (SLANG_FAILED(slang::createGlobalSession(s_global_session.writeRef())))
    {
        PHX_LOG_ERROR(k_log, "Failed to create Slang global session");
        return false;
    }
    
    const rhi::RenderDeviceCapabilities device_caps = rhi::GetRenderDeviceCapabilities();

    slang::CompilerOptionEntry options[] = {
        { slang::CompilerOptionName::EmitSpirvDirectly,         { slang::CompilerOptionValueKind::Int, 1 } },
        { slang::CompilerOptionName::VulkanUseEntryPointName,   { slang::CompilerOptionValueKind::Int, 1 } },
        { slang::CompilerOptionName::Optimization,              { slang::CompilerOptionValueKind::Int, static_cast<int32_t>(ToSlangOptimizationLevel(params.optimization)) } },
        { slang::CompilerOptionName::DebugInformation,          { slang::CompilerOptionValueKind::Int, static_cast<int32_t>(ToSlangDebugInfoLevel(params.debug_info)) } },
        { slang::CompilerOptionName::SPIRVResourceHeapStride,   { slang::CompilerOptionValueKind::Int, static_cast<int32_t>(device_caps.image_descriptor_size) } },
        { slang::CompilerOptionName::SPIRVSamplerHeapStride,    { slang::CompilerOptionValueKind::Int, static_cast<int32_t>(device_caps.sampler_descriptor_size) } },
        { slang::CompilerOptionName::Capability,                { slang::CompilerOptionValueKind::String, 0, 0, "spvDescriptorHeapEXT" } },
    };

    slang::TargetDesc target       = {
        .format                  = SLANG_SPIRV,
        .profile                 = s_global_session->findProfile("spirv_1_6"),
        .compilerOptionEntries   = options,
        .compilerOptionEntryCount = PHX_ARRAY_COUNT(options),
    };

    slang::SessionDesc session_desc      = {
        .targets                    = &target,
        .targetCount                = 1,
        .defaultMatrixLayoutMode    = ToSlangMatrixLayout(params.matrix_layout),
        .searchPaths                = params.shader_search_paths.data(),
        .searchPathCount            = static_cast<SlangInt>(params.shader_search_paths.size()),
        .fileSystem                 = &s_vfs_file_system,
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

Result<MemoryBuffer> phx::ShaderCompiler::Compile(const char* virtual_path, const char* entry_point, Stage stage, const ShaderVariant& variant)
{
    MemoryBuffer source = VFS::ReadFile(virtual_path);
    if (source.IsEmpty())
    {
        PHX_LOG_ERROR(k_log, "Could not read shader source '{0}'", virtual_path);
        return Unexpected(ResultError::NotFound);
    }

    return Compile(source, virtual_path, entry_point, stage, variant);
}


Result<MemoryBuffer> phx::ShaderCompiler::Compile(
    const MemoryBuffer& source,
    const char* virtual_path,
    const char* entry_point,
    Stage stage,
    const ShaderVariant& variant)
{
    slang::IModule* module = LoadModule(source, virtual_path);
    if (!module)
        return Unexpected(ResultError::Failure);

    Slang::ComPtr<slang::IBlob> diagnostics;
    Slang::ComPtr<slang::IEntryPoint> entry;
    if (SLANG_FAILED(module->findAndCheckEntryPoint(entry_point, ToSlangStage(stage), entry.writeRef(), diagnostics.writeRef())))
    {
        LogDiagnostics(diagnostics);
        PHX_LOG_ERROR(k_log, "Entry point '{0}' not found in '{1}'", entry_point, virtual_path);
        return Unexpected(ResultError::NotFound);
    }
 
    const bool generic = IsGeneric(entry);
    if ( generic && !variant.HasArgs())
    {
        PHX_LOG_ERROR(k_log, "'{0}' is {1} but the variant {2} type arguments",
                    entry_point, generic ? "generic" : "not generic",
                    variant.HasArgs() ? "has" : "has no");
        return Unexpected(ResultError::Failure);
    }

    Slang::ComPtr<slang::IComponentType> specialized_entry;
    specialized_entry = entry.get();
    if (generic)
    {
        std::vector<slang::SpecializationArg> args;
        std::vector<Slang::ComPtr<slang::IModule>> arg_modules;

        args.reserve(variant.type_args.Size());
        arg_modules.reserve(variant.type_args.Size());

        for (const ShaderTypeArg& arg : variant.type_args)
        {
            Slang::ComPtr<slang::IModule> arg_module;
            arg_module = s_session->loadModule(arg.module_name, diagnostics.writeRef());
            LogDiagnostics(diagnostics);

            if (!arg_module)
            {
                PHX_LOG_ERROR(
                    k_log,
                    "Failed to load specialization module '{0}'",
                    arg.module_name);

                return Unexpected(ResultError::NotFound);
            }

            // Find the concrete type in that module.
            slang::TypeReflection* type =
                module->getLayout()->findTypeByName(arg.type_name);;

            if (!type)
            {
                PHX_LOG_ERROR(
                    k_log,
                    "Type '{0}' not found in specialization module '{1}'",
                    arg.type_name,
                    arg.module_name);

                return Unexpected(ResultError::NotFound);
            }

            args.push_back(slang::SpecializationArg::fromType(type));

            // Keep the module alive while the specialization is being created.
            arg_modules.push_back(arg_module);
        }
    

        if (SLANG_FAILED(entry->specialize(
                args.data(),
                static_cast<SlangInt>(args.size()),
                specialized_entry.writeRef(),
                diagnostics.writeRef())))
        {
            LogDiagnostics(diagnostics);

            PHX_LOG_ERROR(
                k_log,
                "Failed to specialize '{0}'",
                entry_point);

            return Unexpected(ResultError::Failure);
        }
    }

    slang::IComponentType* components[] = { module, specialized_entry };

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

Result<MemoryBuffer> phx::ShaderCompiler::CompileModule(const char* virtual_path)
{
    MemoryBuffer source = VFS::ReadFile(virtual_path);
    if (source.IsEmpty())
    {
        PHX_LOG_ERROR(k_log, "Could not read shader source '{0}'", virtual_path);
        return Unexpected(ResultError::NotFound);
    }

    return CompileModule(source, virtual_path);
}

Result<MemoryBuffer> phx::ShaderCompiler::CompileModule(const MemoryBuffer& source, const char* virtual_path)
{
    slang::IModule* module = LoadModule(source, virtual_path);
    if (!module)
        return Unexpected(ResultError::Failure);

    const SlangInt32 entry_point_count = module->getDefinedEntryPointCount();
    if (entry_point_count == 0)
    {
        PHX_LOG_ERROR(k_log, "No [shader(...)] entry points found in '{0}'", virtual_path);
        return Unexpected(ResultError::NotFound);
    }

    std::vector<Slang::ComPtr<slang::IEntryPoint>> entry_points(entry_point_count);
    std::vector<slang::IComponentType*> components;
    components.reserve(entry_point_count + 1);
    components.push_back(module);

    for (SlangInt32 i = 0; i < entry_point_count; ++i)
    {
        if (SLANG_FAILED(module->getDefinedEntryPoint(i, entry_points[i].writeRef())))
        {
            PHX_LOG_ERROR(k_log, "Failed to get entry point {0} from '{1}'", i, virtual_path);
            return Unexpected(ResultError::Failure);
        }
        
        components.push_back(entry_points[i]);
    }

    Slang::ComPtr<slang::IBlob> diagnostics;
    Slang::ComPtr<slang::IComponentType> program;
    if (SLANG_FAILED(s_session->createCompositeComponentType(components.data(), static_cast<SlangInt>(components.size()), program.writeRef(), diagnostics.writeRef())))
    {
        LogDiagnostics(diagnostics);
        PHX_LOG_ERROR(k_log, "Failed to link '{0}'", virtual_path);
        return Unexpected(ResultError::Failure);
    }
    
    Slang::ComPtr<slang::IBlob> code;
    if (SLANG_FAILED(program->getTargetCode(0, code.writeRef(), diagnostics.writeRef())))
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
