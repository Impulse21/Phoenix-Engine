#include "PhxRenderer_pch.h"

#include <PhxRenderer/Shaders/ShaderSystemTypes.h>
#include "ShaderSystemTypes.h"

using namespace phx;
using namespace phx::renderer;

Hash64 ShaderDescriptor::GetHash() const
{    
    std::size_t seed = 0;

    HashCombine(seed, virtual_path);

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