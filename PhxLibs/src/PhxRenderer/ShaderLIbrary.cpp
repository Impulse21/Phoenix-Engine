#include "PhxRenderer/PhxRenderer_pch.h"
#include "ShaderLIbrary.h"

#include <PhxCore/HashHelper.h>


using namespace phx;
using namespace phx::renderer;


void phx::renderer::ShaderLibrary::Initialize(std::vector<std::string>&& include_paths)
{
	m_include_paths = std::move(include_paths);
}

void phx::renderer::ShaderLibrary::Shutdown()
{
	std::scoped_lock _(m_cache_mutex);

	m_cached_compile_desc.clear();
	m_cached_assets.clear();
}

bool phx::renderer::ShaderLibrary::LoadShader(ShaderCompileDescriptor const& compile_desc)
{
	return false;
}

void phx::renderer::ShaderLibrary::ReloadAll()
{
}

uint64_t phx::renderer::ShaderCompileDescriptor::GetHash() const
{
    std::size_t seed = 0;

    HashCombine(seed, source_file_path);
    HashCombine(seed, (uint32_t)target);
    HashCombine(seed, debug_info);
    HashCombine(seed, optimization);

    // 2. Hash Entry Points (Order matters, so just loop)
    for (const auto& ep : entry_points)
    {
        HashCombine(seed, ep.name);
        HashCombine(seed, (uint32_t)ep.stage);
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
