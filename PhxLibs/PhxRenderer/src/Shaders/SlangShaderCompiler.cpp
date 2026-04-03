#include "PhxRenderer_pch.h"

#include "SlangShaderCompiler.h"
#include <PhxCore/IO/FileUtils.h>
#include <PhxRhi/PhxRhi.h>

using namespace phx;
using namespace phx::renderer;

namespace
{
    void LogSlangDiagnostics(slang::IBlob* blob, const std::string& context)
    {
        if (blob)
        {
            std::string_view msg((const char*)blob->getBufferPointer(), blob->getBufferSize());
            PHX_CORE_ERROR("Slang error[{0}] {1}", context, msg);
        }
    }

    constexpr rhi::ShaderStage MapSlangStage(SlangStage stage)
    {
        switch (stage)
        {
        case SLANG_STAGE_VERTEX:        return rhi::ShaderStage::VS;
        case SLANG_STAGE_FRAGMENT:      return rhi::ShaderStage::PS;
        case SLANG_STAGE_COMPUTE:       return rhi::ShaderStage::CS;

        case SLANG_STAGE_GEOMETRY:      return rhi::ShaderStage::GS;

        case SLANG_STAGE_HULL:          return rhi::ShaderStage::HS;
        case SLANG_STAGE_DOMAIN:        return rhi::ShaderStage::DS;

        case SLANG_STAGE_AMPLIFICATION: return rhi::ShaderStage::AS;
        case SLANG_STAGE_MESH:          return rhi::ShaderStage::MS;

        default:
            // Fallback or Error
            return rhi::ShaderStage::Count;
        }
    }
}

namespace
{
    Slang::ComPtr<slang::IGlobalSession> g_global_session;
    thread_local std::unique_ptr<SlangCompilerSession> g_thread_local_session;
}

void phx::renderer::SlangCompiler::Initialize()
{
	PHX_CORE_INFO("Creating Slang Global Session...");
    if (SLANG_FAILED(slang::createGlobalSession(g_global_session.writeRef())))
    {
        PHX_CORE_ERROR("Critical Error: Failed to create Slang Global Session.");
        return;
    }
}

void phx::renderer::SlangCompiler::Shutdown()
{
    g_global_session.setNull();
}

SlangCompilerSession* phx::renderer::SlangCompiler::GetOrCreateCompilerSession()
{
    if (!g_thread_local_session)
    {
        g_thread_local_session= SlangCompiler::CreateCompileSession({
            .vfs = IVirtualFileSystem::Ptr,
            .target = rhi::GetShaderFormat(),
            .include_paths = {},
            .defines = {},
    });

    return g_thread_local_session.get();
}

std::unique_ptr<SlangCompilerSession> phx::renderer::SlangCompiler::CreateCompileSession(const SlangCompilerSessionDescriptor& desc)
{
    slang::SessionDesc session_desc = {};

    // -- set search paths -- 
    std::vector<const char*> search_paths;
    search_paths.reserve(desc.include_paths.size());

    for (const auto& path : desc.include_paths)
        search_paths.push_back(path.c_str());

    session_desc.searchPaths = search_paths.data();
    session_desc.searchPathCount = (SlangInt)search_paths.size();

    // -- Set Macros ---
    std::vector<slang::PreprocessorMacroDesc> macros;
    for (const auto& [key, val] : desc.defines)
    {
        macros.push_back({ key.c_str(), val.c_str() });
    }

    session_desc.preprocessorMacros = macros.data();
    session_desc.preprocessorMacroCount = (SlangInt)macros.size();

    session_desc.defaultMatrixLayoutMode = desc.force_column_major
        ? SLANG_MATRIX_LAYOUT_COLUMN_MAJOR
        : SLANG_MATRIX_LAYOUT_ROW_MAJOR;

    // -- Setting target desc ---
    slang::TargetDesc target_desc = {};

    switch (desc.target)
    {
    case rhi::ShaderFormat::Spirv:
    {
		PHX_CORE_INFO("Setting Slang Target to SPIR-V");
        target_desc.format = SLANG_SPIRV;
        target_desc.flags |= SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;
        target_desc.profile = g_global_session->findProfile("spirv_1_6");

        break;
    }

    case rhi::ShaderFormat::Hlsl6:
    {
        PHX_CORE_INFO("Setting Slang Target to DXIL");
        target_desc.format = SLANG_DXIL;
        target_desc.profile = g_global_session->findProfile("sm_6_6");
        break;
    }
    }

    // -- Setting compiler options ---
    std::vector<slang::CompilerOptionEntry> options;

    // Optimization Level
    {
        slang::CompilerOptionEntry opt = {};
        opt.name = slang::CompilerOptionName::Optimization;
        opt.value.kind = slang::CompilerOptionValueKind::Int;
        opt.value.intValue0 = desc.optimization ? SLANG_OPTIMIZATION_LEVEL_HIGH : SLANG_OPTIMIZATION_LEVEL_NONE;
        options.push_back(opt);
    }

    {
        slang::CompilerOptionEntry opt = {};
        opt.name = slang::CompilerOptionName::VulkanUseEntryPointName;
        opt.value.kind = slang::CompilerOptionValueKind::Int;
        opt.value.intValue0 = 1;
        options.push_back(opt);
    }

    // Debug Info
    if (desc.debug_info)
    {
        slang::CompilerOptionEntry opt = {};
        opt.name = slang::CompilerOptionName::DebugInformation;
        opt.value.kind = slang::CompilerOptionValueKind::Int;
        opt.value.intValue0 = SLANG_DEBUG_INFO_LEVEL_STANDARD;
        options.push_back(opt);
    }

    target_desc.compilerOptionEntries = options.data();
    target_desc.compilerOptionEntryCount = (uint32_t)options.size();

    session_desc.targets = &target_desc;
    session_desc.targetCount = 1;

    std::unique_ptr<SlangCompilerSession>  session = std::make_unique<SlangCompilerSession>(session_desc, g_global_session, desc.vfs);

    session->Initialize();
    
    return session;
}

phx::renderer::SlangCompilerSession::SlangCompilerSession(
    const slang::SessionDesc &slang_desc,
    Slang::ComPtr<slang::IGlobalSession> global_session,
    IVirtualFileSystem* vfs)
    : m_slang_session(nullptr)
    , m_slang_desc(slang_desc)
    , m_global_session(global_session)
    , m_vfs(vfs)
{
}

void phx::renderer::SlangCompilerSession::Initialize()
{
    if (SLANG_FAILED(m_global_session->createSession(m_slang_desc, m_slang_session.writeRef())))
    {
        PHX_CORE_ERROR("Failed to create SLANG session");
    }
}

Slang::ComPtr<slang::IModule> phx::renderer::SlangCompilerSession::LoadModule(const std::string& virtual_path)
{
    auto itr = m_loaded_modules.find(virtual_path);
    if (itr != m_loaded_modules.end())
        return itr->second;

    auto vfs = phx::IVirtualFileSystem::Ptr;

    FilePtr file = vfs->Open(virtual_path, FileMode::Read);
    if (!file)
    {
        PHX_CORE_ERROR("AssetDB: failed to open '{0}'", virtual_path);
        return nullptr;
    }

    std::string content(file->GetSize(), '\0');
    file->Read(content.data(), content.size());

    std::string module_name = phx::GetFileNameWithoutExt(virtual_path);
    
    // TODO: we are checking the cache twice here.
    return LoadModule(module_name, virtual_path, content.data(), content.size());
}

Slang::ComPtr<slang::IModule> phx::renderer::SlangCompilerSession::LoadModule(const std::string& module_name, const std::string& virtual_path, const void* data, size_t data_size)
{

    auto itr = m_loaded_modules.find(virtual_path);
    if (itr != m_loaded_modules.end())
        return itr->second;


	Slang::ComPtr<ISlangBlob> slang_shader_blob(slang_createBlob(data, data_size));
	Slang::ComPtr<slang::IBlob> diagnostic_blob;
	Slang::ComPtr<slang::IModule> shader_module = nullptr;

	{
		shader_module = m_slang_session->loadModuleFromSource(
			module_name.c_str(),
			virtual_path.c_str(),
			slang_shader_blob,
			diagnostic_blob.writeRef());
	}

	if (!shader_module)
	{
		LogSlangDiagnostics(diagnostic_blob, virtual_path);
	}
    else
    {
        m_loaded_modules[virtual_path] = shader_module;
    }

    return shader_module;
}

RefCountPtr<SlangShader> phx::renderer::SlangCompilerSession::LinkProgram(const ShaderDescriptor shader_desc)
{
    // -- Load module checks cache ---
    Slang::ComPtr<slang::IModule> loaded_moduled = LoadModule(shader_desc.virtual_path);

    Slang::ComPtr<slang::IComponentType> composed_program;
    const bool compile_full_module = shader_desc.entry_points.empty();

    Slang::ComPtr<slang::IBlob> diagnostic_blob;
    if (compile_full_module)
    {
        composed_program = loaded_moduled;
    }
    else
    {
        std::vector<slang::IComponentType*> components;
        components.push_back(loaded_moduled);

        std::vector<Slang::ComPtr<slang::IEntryPoint>> keep_alive;
        for (const auto& ep : shader_desc.entry_points)
        {
            Slang::ComPtr<slang::IEntryPoint> entry_point;
            loaded_moduled->findEntryPointByName(ep.name.c_str(), entry_point.writeRef());

            if (!entry_point)
            {
                PHX_CORE_ERROR("Entry point {0} not found in '{1}'", ep.name, shader_desc.virtual_path);
                return nullptr;
            }

            components.push_back(entry_point);
            keep_alive.push_back(entry_point);
        }

        diagnostic_blob = nullptr;

        // createCompositeComponentType can be skipped if just using all entry points;

        m_slang_session->createCompositeComponentType(
            components.data(),
            components.size(),
            composed_program.writeRef(),
            diagnostic_blob.writeRef());
        if (!composed_program)

        {
            LogSlangDiagnostics(diagnostic_blob, "Failed to compose");
            return nullptr;
        }
    }

    diagnostic_blob = nullptr;
    Slang::ComPtr<slang::IComponentType> linked_programs;
    SlangResult result = composed_program->link(
        linked_programs.writeRef(),
        diagnostic_blob.writeRef());

    if (SLANG_FAILED(result))
    {
        LogSlangDiagnostics(diagnostic_blob, "Failed to link");
        return nullptr;
    }

    Slang::ComPtr<slang::IBlob> code_blob;
    diagnostic_blob = nullptr;
    SlangResult res = linked_programs->getTargetCode(
        0,
        code_blob.writeRef(),
        diagnostic_blob.writeRef());

    if (SLANG_FAILED(res))
    {
        LogSlangDiagnostics(diagnostic_blob, "Failed to generate code blob");
        return nullptr;
    }

    auto shader = RefCountPtr<SlangShader>::Create();
    shader->linked_programs = linked_programs;
    shader->code_blob = code_blob;

    slang::ProgramLayout* layout = linked_programs->getLayout();
    uint32_t ep_count = (uint32_t)layout->getEntryPointCount();

    for (uint32_t i = 0; i < ep_count; ++i)
    {
        auto* entryPointRef = layout->getEntryPointByIndex(i);

        // Get the real name (will remain "vsMain" because of the Option we set)
        const char* real_name = entryPointRef->getNameOverride();
        if (!real_name) real_name = entryPointRef->getName();

        // Map Slang Stage to RHI Stage
        rhi::ShaderStage stage = MapSlangStage(entryPointRef->getStage());

        // Store in our SlangShader manifest
        shader->entry_points.push_back({ std::string(real_name), stage });
    }

    return shader;
}
