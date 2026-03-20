#pragma once

#include <string>
#include <variant>
#include <optional>
#include <PhxCore/Reflect/Reflection.h>
#include <PhxAsset/AssetTypes.h>

namespace phx::renderer::assets
{
    struct Float2 { float x = 0.f, y = 0.f; };
    struct Float3 { float x = 0.f, y = 0.f, z = 0.f; };
    struct Float4 { float x = 0.f, y = 0.f, z = 0.f, w = 0.f; };

    using ParamValue = std::variant
    <
        rfl::Field<"float4",  Float4>,
        rfl::Field<"float3",  Float3>,
        rfl::Field<"float2",  Float2>,
        rfl::Field<"bool",    bool>,
        rfl::Field<"float",   float>,
        rfl::Field<"int",     int32_t>,
        rfl::Field<"texture", std::string>
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
        std::optional<std::string> pixel_shader;
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