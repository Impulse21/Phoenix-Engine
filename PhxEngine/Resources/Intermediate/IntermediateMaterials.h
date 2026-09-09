#pragma once

#include <hlsl++.h>

#include <string>

namespace phx::resources
{
    enum class MaterialDomain : uint8_t
    {
        Opaque,
        Masked,
        Transparent,
    };

    struct IntermediateMaterial
    {
        std::string name;
        std::string archetype = "standard_pbr";

        MaterialDomain domain = MaterialDomain::Opaque;
        bool double_sided = false;

        hlslpp::float4 base_color_factor = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);
        hlslpp::float3 emissive_factor   = hlslpp::float3(0.0f, 0.0f, 0.0f);

        float metallic_factor     = 1.0f;
        float roughness_factor    = 1.0f;
        float normal_scale        = 1.0f;
        float occlusion_strength  = 1.0f;
        float alpha_cutoff        = 0.5f;

        static constexpr uint32_t kInvalidTextureIndex = ~0u;
        uint32_t base_color_texture         = kInvalidTextureIndex;
        uint32_t normal_texture             = kInvalidTextureIndex;
        uint32_t metallic_roughness_texture = kInvalidTextureIndex;
        uint32_t occlusion_texture          = kInvalidTextureIndex;
        uint32_t emissive_texture           = kInvalidTextureIndex;
    };
}
