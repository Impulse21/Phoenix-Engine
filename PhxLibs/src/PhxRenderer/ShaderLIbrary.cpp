#include "PhxRenderer/PhxRenderer_pch.h"
#include "ShaderLibrary.h"

#include <PhxCore/IVirtualFileSystem.h>

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
}

void phx::renderer::ShaderLibrary::Initialize(const ShaderLibraryDescriptor& librar_desc)
{
    if (SLANG_FAILED(slang::createGlobalSession(m_global_session.writeRef())))
    {
        PHX_CORE_ERROR("Critical Error: Failed to create Slang Global Session.");
        return;
    }

    m_library_desc = librar_desc;
    ConstructSession();
}

void phx::renderer::ShaderLibrary::Shutdown()
{
	std::scoped_lock _(m_cache_mutex);

	m_cached_compile_desc.clear();
	m_cached_assets.clear();
}

RefCountPtr<ShaderAsset> phx::renderer::ShaderLibrary::LoadShader(ShaderCompileDescriptor const& compile_desc)
{
    const Hash64 cache_key = compile_desc.GetHash();
    {
        std::scoped_lock _(m_cache_mutex);
        auto it = m_cached_assets.find(cache_key);
        if (it != m_cached_assets.end())
        {
            return it->second;
        }
    }

    RefCountPtr<SlangShader> rawShader = Compile(compile_desc);

    if (!rawShader)
    {
        return nullptr;
    }

    auto new_asset = RefCountPtr<ShaderAsset>::Create();
    new_asset->m_current= rawShader;
    new_asset->m_src_path = compile_desc.source_file_path;

    {
        std::scoped_lock _(m_cache_mutex);
        m_cached_assets[cache_key] = new_asset;
        m_cached_compile_desc[cache_key] = compile_desc;
    }

    return new_asset;
}

void phx::renderer::ShaderLibrary::ReloadAll()
{
    ConstructSession();
}

RefCountPtr<SlangShader> phx::renderer::ShaderLibrary::Compile(ShaderCompileDescriptor const& compile_desc)
{
    PHX_ASSERT(m_session, "Initialize wasn't called");

    Result<std::string> physical_path = IVirtualFileSystem::Ptr->ResolveVirtualToPhysicalPath(compile_desc.source_file_path);
    Slang::ComPtr<slang::IBlob> diagnostic_blob;
    slang::IModule* shader_module = m_session->loadModule(physical_path.GetValue().c_str(), diagnostic_blob.writeRef());

    if (!shader_module)
    {
        LogSlangDiagnostics(diagnostic_blob, compile_desc.source_file_path);
        return nullptr;
    }

    Slang::ComPtr<slang::IComponentType> composed_program;
    const bool compile_full_module = compile_desc.entry_points.empty();

    if (compile_full_module)
    {
        composed_program = shader_module;
    }
    else
    {
        std::vector<Slang::ComPtr<slang::IEntryPoint>> entry_points;
        std::vector<slang::IComponentType*> components;

        components.push_back(shader_module);
        for (const auto& ep : compile_desc.entry_points)
        {
            Slang::ComPtr<slang::IEntryPoint> entry_point;
            shader_module->findEntryPointByName(ep.name.c_str(), entry_point.writeRef());

            if (!entry_point)
            {
                PHX_CORE_ERROR("Entry point {0} not found in '{1}'", ep.name, compile_desc.source_file_path);
                return nullptr;
            }

            components.push_back(entry_point);
            entry_points.push_back(entry_point);
        }

        diagnostic_blob = nullptr;

        // createCompositeComponentType can be skipped if just using all entry points;
        m_session->createCompositeComponentType(
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
    SlangResult res = 0;
	if (compile_full_module)
	{
		res = linked_programs->getTargetCode(
			0,
			code_blob.writeRef(),
			diagnostic_blob.writeRef());
	}
	else
    {
        res = linked_programs->getEntryPointCode(
            0,
            0,
            code_blob.writeRef(),
            diagnostic_blob.writeRef());
    } 

    if (SLANG_FAILED(res))
    {
        PHX_CORE_ERROR("Failed to generate code blob");
        return nullptr;
    }

    auto shader = RefCountPtr<SlangShader>::Create();
    shader->m_linked_programs = linked_programs;
    shader->m_code_blob = code_blob;

    return shader;
}

void phx::renderer::ShaderLibrary::ConstructSession()
{
    PHX_ASSERT(m_global_session, "Shader Library was never Initialized");

    slang::SessionDesc session_desc = {};

    std::vector<const char*> search_paths;
    search_paths.reserve(m_library_desc.include_paths.size());
    for (const auto& path : m_library_desc.include_paths)
        search_paths.push_back(path.c_str());

    session_desc.searchPaths = search_paths.data();
    session_desc.searchPathCount = (SlangInt)search_paths.size();

    std::vector<slang::PreprocessorMacroDesc> macros;
    for (const auto& [key, val] : m_library_desc.defines)
    {
        macros.push_back({ key.c_str(), val.c_str() });
    }

    session_desc.preprocessorMacros = macros.data();
    session_desc.preprocessorMacroCount = (SlangInt)macros.size();

    session_desc.defaultMatrixLayoutMode = m_library_desc.ForceColumnMajor 
        ? SLANG_MATRIX_LAYOUT_COLUMN_MAJOR
        : SLANG_MATRIX_LAYOUT_ROW_MAJOR;

    slang::TargetDesc target_desc = {};

    switch (m_library_desc.target)
    {
    case rhi::ShaderFormat::Spirv:
        target_desc.format = SLANG_SPIRV;
        target_desc.flags |= SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;
        target_desc.profile = m_global_session->findProfile("spirv_1_6");

        break;
    case rhi::ShaderFormat::Hlsl6:
        target_desc.format = SLANG_DXIL;
        target_desc.profile = m_global_session->findProfile("sm_6_6");
        break;
    }

    std::vector<slang::CompilerOptionEntry> options;

    // Optimization Level
    {
        slang::CompilerOptionEntry opt = {};
        opt.name = slang::CompilerOptionName::Optimization;
        opt.value.kind = slang::CompilerOptionValueKind::Int;
        opt.value.intValue0 = m_library_desc.optimization ? SLANG_OPTIMIZATION_LEVEL_HIGH : SLANG_OPTIMIZATION_LEVEL_NONE;
        options.push_back(opt);
    }

    // Debug Info
    if (m_library_desc.debug_info)
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

    m_session = nullptr;
    if (SLANG_FAILED(m_global_session->createSession(session_desc, m_session.writeRef())))
    {
        PHX_CORE_ERROR("Failed to create SLANG session");
    }
}

Hash64 phx::renderer::ShaderCompileDescriptor::GetHash() const
{
    std::size_t seed = 0;

    HashCombine(seed, source_file_path);

    std::vector<const EntryPoint*> sorted_entries;
    sorted_entries.reserve(entry_points.size());
    for (const auto& ep : entry_points)
    {
        sorted_entries.push_back(&ep);
    }

    // Sort by Name (or Stage) to ensure VS+PS hashes same as PS+VS
    std::sort(sorted_entries.begin(), sorted_entries.end(),
        [](const EntryPoint* a, const EntryPoint* b) {
            return a->name < b->name;
        });

    for (const auto* ep : sorted_entries)
    {
        HashCombine(seed, ep->name);
        HashCombine(seed, (uint32_t)ep->stage);
    }

    std::vector<const GenericArg*> sorted_generic_args;
    sorted_generic_args.reserve(generic_args.size());
    for (const auto& ga : generic_args)
    {
        sorted_generic_args.push_back(&ga);
    }

    // Sort by Name (or Stage) to ensure VS+PS hashes same as PS+VS
    std::sort(sorted_generic_args.begin(), sorted_generic_args.end(),
        [](const GenericArg* a, const GenericArg* b) {
            return a->name < b->name;
        });

    for (const auto* ga : sorted_generic_args)
    {
        HashCombine(seed, ga->name);
        HashCombine(seed, ga->value);
        HashCombine(seed, ga->is_type);
    }

    return (uint64_t)seed;
}

const void* phx::renderer::SlangShader::GetEntryPointCode(int /*entry_point_index*/, size_t& out_size) const
{
    if (m_code_blob)
    {
        out_size = m_code_blob->getBufferSize();
        return m_code_blob->getBufferPointer();
    }

    out_size = 0;
    return nullptr;
}
