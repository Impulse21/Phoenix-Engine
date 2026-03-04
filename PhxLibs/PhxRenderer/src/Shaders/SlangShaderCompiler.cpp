#include "PhxRenderer_pch.h"
#include <PhxRenderer/Shaders/SlangShaderCompiler.h>

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

Slang::ComPtr<slang::IModule> phx::renderer::SlangShaderCompiler::LoadModule(
    const void* slang_data,
    size_t slang_data_size,
    const char* /*module_name*/,
    const char* path)
{
	ISlangBlob* slang_shader_blob = slang_createBlob(slang_data, slang_data_size);
	Slang::ComPtr<slang::IBlob> diagnostic_blob;
	Slang::ComPtr<slang::IModule> shader_module = nullptr;

	{
		// const char* module_compiler_version;
		const char* module_name = "";
		shader_module = ms_session->loadModuleFromSource(
			module_name,
			path,
			slang_shader_blob,
			diagnostic_blob.writeRef());
	}

	if (!shader_module)
	{
		LogSlangDiagnostics(diagnostic_blob, path);
	}

	slang_shader_blob->Release();
	return shader_module;
}

Slang::ComPtr<slang::IModule> phx::renderer::SlangShaderCompiler::LoadModule(const std::string& physical_path, const std::string& /*source*/)
{
    Slang::ComPtr<slang::IBlob> diagnostic_blob;
    Slang::ComPtr<slang::IModule> shader_module = nullptr;
    
    {
		std::scoped_lock lock(ms_mutex);
        shader_module = ms_session->loadModule(physical_path.c_str(), diagnostic_blob.writeRef());
    }

    if (!shader_module)
    {
        LogSlangDiagnostics(diagnostic_blob, physical_path);
    }

    return shader_module;
}

RefCountPtr<SlangShader> phx::renderer::SlangShaderCompiler::Compile(const ShaderCompileDescriptor& compile_desc)
{
	ShaderModuleResource* module_resource = compile_desc.shader_module_resource;

    Slang::ComPtr<slang::IComponentType> composed_program;
    const bool compile_full_module = compile_desc.entry_points.IsEmpty();

    Slang::ComPtr<slang::IBlob> diagnostic_blob;
    if (compile_full_module)
    {
        composed_program = module_resource->slang_module;
    }
    else
    {
        std::vector<slang::IComponentType*> components;
        components.push_back(module_resource->slang_module);

        std::vector<Slang::ComPtr<slang::IEntryPoint>> keep_alive;
        for (const auto& ep : compile_desc.entry_points)
        {
            Slang::ComPtr<slang::IEntryPoint> entry_point;
            module_resource->slang_module->findEntryPointByName(ep.name.c_str(), entry_point.writeRef());

            if (!entry_point)
            {
                PHX_CORE_ERROR("Entry point {0} not found in '{1}'", ep.name, module_resource->source_path);
                return nullptr;
            }

            components.push_back(entry_point);
            keep_alive.push_back(entry_point);
        }

        diagnostic_blob = nullptr;

        // createCompositeComponentType can be skipped if just using all entry points;
        {
            std::scoped_lock lock(ms_mutex);
            ms_session->createCompositeComponentType(
                components.data(),
                components.size(),
                composed_program.writeRef(),
                diagnostic_blob.writeRef());


        }
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

void SlangShaderCompiler::InitializeSlang()
{
	PHX_CORE_INFO("Creating Slang Global Session...");
    if (SLANG_FAILED(slang::createGlobalSession(ms_global_session.writeRef())))
    {
        PHX_CORE_ERROR("Critical Error: Failed to create Slang Global Session.");
        return;
    }

    slang::SessionDesc session_desc = {};

    // -- set search paths -- 
    std::vector<const char*> search_paths;
    search_paths.reserve(ms_create_info.include_paths.size());

    for (const auto& path : ms_create_info.include_paths)
        search_paths.push_back(path.c_str());

    session_desc.searchPaths = search_paths.data();
    session_desc.searchPathCount = (SlangInt)search_paths.size();

    // -- Set Macros ---
    std::vector<slang::PreprocessorMacroDesc> macros;
    for (const auto& [key, val] : ms_create_info.defines)
    {
        macros.push_back({ key.c_str(), val.c_str() });
    }

    session_desc.preprocessorMacros = macros.data();
    session_desc.preprocessorMacroCount = (SlangInt)macros.size();

    session_desc.defaultMatrixLayoutMode = ms_create_info.force_column_major
        ? SLANG_MATRIX_LAYOUT_COLUMN_MAJOR
        : SLANG_MATRIX_LAYOUT_ROW_MAJOR;

    // -- Setting target desc ---
    slang::TargetDesc target_desc = {};

    switch (ms_create_info.target)
    {
    case rhi::ShaderFormat::Spirv:
    {
		PHX_CORE_INFO("Setting Slang Target to SPIR-V");
        target_desc.format = SLANG_SPIRV;
        target_desc.flags |= SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;
        target_desc.profile = ms_global_session->findProfile("spirv_1_6");

        break;
    }

    case rhi::ShaderFormat::Hlsl6:
    {
        PHX_CORE_INFO("Setting Slang Target to DXIL");
        target_desc.format = SLANG_DXIL;
        target_desc.profile = ms_global_session->findProfile("sm_6_6");
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
        opt.value.intValue0 = ms_create_info.optimization ? SLANG_OPTIMIZATION_LEVEL_HIGH : SLANG_OPTIMIZATION_LEVEL_NONE;
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
    if (ms_create_info.debug_info)
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

    ms_session = nullptr;
    if (SLANG_FAILED(ms_global_session->createSession(session_desc, ms_session.writeRef())))
    {
        PHX_CORE_ERROR("Failed to create SLANG session");
    }
}

#if false

// Simple hash combine function (Keeping around for reference)
Hash64 phx::renderer::ShaderCompileDescriptor::GetHash() const
{
    std::size_t seed = 0;

    HashCombine(seed, source_file_path);

    std::vector<const ShaderEntryPoint*> sorted_entries;
    sorted_entries.reserve(entry_points.size());
    for (const auto& ep : entry_points)
    {
        sorted_entries.push_back(&ep);
    }

    // Sort by Name (or Stage) to ensure VS+PS hashes same as PS+VS
    std::sort(sorted_entries.begin(), sorted_entries.end(),
        [](const ShaderEntryPoint* a, const ShaderEntryPoint* b) {
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

#endif