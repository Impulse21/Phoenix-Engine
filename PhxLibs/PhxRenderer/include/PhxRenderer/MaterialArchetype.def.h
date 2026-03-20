#pragma once

#include <string>
#include <variant>
#include <PhxCore/Reflect/Reflection.h>
#include <PhxAsset/AssetTypes.h>

namespace phx::renderer::assets
{
    using ParamValue = std::variant
    <
        rfl::Rename<"float",   float>,
        rfl::Rename<"float2",  std::array<float, 2>>,
        rfl::Rename<"float3",  std::array<float, 3>>,
        rfl::Rename<"float4",  std::array<float, 4>>,
        rfl::Rename<"bool",    bool>,
        rfl::Rename<"int",     int32_t>,
        rfl::Rename<"texture", std::string>
    >;

    struct MaterialInstanceParam
    {
        PHX_DECLARE_REFLECT(MaterialInstanceParam)

        std::string name;
        ParamValue value;
    };

    struct MaterialTechnique
    {
        PHX_DECLARE_REFLECT(MaterialTechnique)

        std::string name;
        std::string vertex_shader;
        std::string pixel_shader;
    };

    struct BlendStateDesc
    {
        PHX_DECLARE_REFLECT(BlendStateDesc)

        std::string src_factor  = "One";
        std::string dst_factor  = "Zero";
        std::string blend_op    = "Add";
    };

    struct ArchetypeRenderState
    {
        PHX_DECLARE_REFLECT(ArchetypeRenderState)

        std::string     cull_mode       = "Back";
        std::string     front_face      = "CounterClockwise";
        bool            depth_test      = true;
        bool            depth_write     = true;
        std::string     depth_compare   = "LessOrEqual";
        BlendStateDesc  blend           = {};
    };

    struct ShaderGeneric
    {
        PHX_DECLARE_REFLECT(ShaderGeneric)
        std::string name;
        std::string value;
    };

    struct ShaderDescriptor
    {
        PHX_DECLARE_REFLECT(ShaderDescriptor)

        std::string                  source;
        std::vector<ShaderGeneric>   generics;
    };

    struct MaterialArchetypeDef
    {
        PHX_DEFINE_ASSET(MaterialArchetypeDef);

        ShaderDescriptor                    shader_desc;
        bool                                is_double_sided = false;
        ArchetypeRenderState                render_state;
        std::vector<MaterialTechnique>      techniques;
        std::vector<MaterialInstanceParam>  params;
    };

    struct MaterialInstaceDef
    {
        PHX_DEFINE_ASSET(MaterialInstaceDef);
    };
}