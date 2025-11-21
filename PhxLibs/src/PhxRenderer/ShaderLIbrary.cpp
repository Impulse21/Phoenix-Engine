#include "PhxRenderer/PhxRenderer_pch.h"
#include "ShaderLIbrary.h"

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
            PHX_CORE_ERROR("Slang error -> {0}", context);
        }
    }
}

void phx::renderer::ShaderLibrary::Initialize(std::vector<std::string>&& include_paths)
{
    if (SLANG_FAILED(slang::createGlobalSession(m_global_session.writeRef())))
    {
        PHX_CORE_ERROR("Critical Error: Failed to create Slang Global Session.");
        return;
    }

    PHX_CORE_INFO("Initialized with {0} include paths.", include_paths.size());

	m_include_paths = std::move(include_paths);
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
            return it->second; // Cache Hit! Return existing proxy.
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
}

RefCountPtr<SlangShader> phx::renderer::ShaderLibrary::Compile(ShaderCompileDescriptor const& compile_desc)
{
    PHX_ASSERT(m_global_session, "Shader Library was never Initialized");

    slang::SessionDesc session_desc = {};

    std::vector<const char*> search_paths;
    search_paths.reserve(m_include_paths.size());
    for (const auto& path : m_include_paths) 
        search_paths.push_back(path.c_str());

    session_desc.searchPaths = search_paths.data();
    session_desc.searchPathCount = (SlangInt)search_paths.size();

    std::vector<slang::PreprocessorMacroDesc> macros;
    for (const auto& [key, val] : compile_desc.defines)
    {
        macros.push_back({ key.c_str(), val.c_str() });
    }

    session_desc.preprocessorMacros = macros.data();
    session_desc.preprocessorMacroCount = (SlangInt)macros.size();

    session_desc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

    slang::TargetDesc target_desc= {};

    switch (compile_desc.target)
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
        opt.value.intValue0 = compile_desc.optimization ? SLANG_OPTIMIZATION_LEVEL_HIGH : SLANG_OPTIMIZATION_LEVEL_NONE;
        options.push_back(opt);
    }

    // Debug Info
    if (compile_desc.debug_info)
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

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(m_global_session->createSession(session_desc, session.writeRef())))
    {
        PHX_CORE_ERROR("Failed to create SLANG session");
        return nullptr;
    }

    Result<std::string> physical_path = IVirtualFileSystem::Ptr->ResolveVirtualToPhysicalPath(compile_desc.source_file_path);
    Slang::ComPtr<slang::IBlob> diagnostic_blob;
    slang::IModule* module = session->loadModule(physical_path.GetValue().c_str(), diagnostic_blob.writeRef());

    if (!module)
    {
        LogSlangDiagnostics(diagnostic_blob, compile_desc.source_file_path);
        return nullptr;
    }

    // TODO: Check ref counts
    std::vector<Slang::ComPtr<slang::IEntryPoint>> entry_points;
    std::vector<slang::IComponentType*> components;
    for (const auto& ep : compile_desc.entry_points)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point;
        module->findEntryPointByName(ep.name.c_str(), entry_point.writeRef());

        if (!entry_point)
        {
            PHX_CORE_ERROR("Entry point {0} not found in '{1}'", ep.name, compile_desc.source_file_path);
            return nullptr;
        }

        components.push_back(entry_point);
        entry_points.push_back(entry_point);
    }

    diagnostic_blob = nullptr;

    Slang::ComPtr<slang::IComponentType> linked_programs;
    session->createCompositeComponentType(
        components.data(),
        components.size(),
        linked_programs.writeRef(),
        diagnostic_blob.writeRef());

    if (!linked_programs)
    {
        LogSlangDiagnostics(diagnostic_blob, "Failed to link");
        return nullptr;
    }

    Slang::ComPtr<slang::IBlob> code_blob;
    diagnostic_blob = nullptr;

    SlangResult res = linked_programs->getEntryPointCode(
        0, 
        0, 
        code_blob.writeRef(),
        code_blob.writeRef());

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

Hash64 phx::renderer::ShaderCompileDescriptor::GetHash() const
{
    std::size_t seed = 0;

    HashCombine(seed, source_file_path);
    HashCombine(seed, (uint32_t)target);
    HashCombine(seed, debug_info);
    HashCombine(seed, optimization);

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

    std::vector<const std::pair<std::string, std::string>*> sorted_defines;
    sorted_defines.reserve(defines.size());

    for (const auto& def : defines)
    {
        sorted_defines.push_back(&def);
    }

    // Sort the pointers based on the string keys
    std::sort(sorted_defines.begin(), sorted_defines.end(),
        [](const auto* a, const auto* b) { return a->first < b->first; });

    // Now hash in deterministic order
    for (const auto* def : sorted_defines)
    {
        HashCombine(seed, def->first);
        HashCombine(seed, def->second);
    }

    return (uint64_t)seed;
}
