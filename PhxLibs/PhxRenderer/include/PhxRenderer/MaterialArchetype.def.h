#pragma once

#include <string>
#include <PhxCore/Reflect/Reflection.h>
#include <PhxAsset/AssetPtr.h>

namespace phx::renderer::assets
{
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

    struct MaterialParamFloat
    {
        PHX_DECLARE_REFLECT(MaterialParamFloat)

        std::string name;
        float       value = 0.0f;
    };


    struct MaterialParamFloat2
    {
        PHX_DECLARE_REFLECT(MaterialParamFloat)

        std::string                 name;
        hlslpp::interop::float2     value = hlslpp::interop::float2(0.0f);
    };
    struct MaterialParamFloat3
    {
        PHX_DECLARE_REFLECT(MaterialParamFloat)

        std::string                 name;
        hlslpp::interop::float3     value = hlslpp::interop::float3(0.0f);
    };

    struct MaterialParamFloat4
    {
        PHX_DECLARE_REFLECT(MaterialParamFloat)

        std::string                 name;
        hlslpp::interop::float4     value = hlslpp::interop::float4(0.0f);
    };

    struct MaterialParamInt
    {
        PHX_DECLARE_REFLECT(MaterialParamFloat)

        std::string name;
        int         value = 0;
    };

    struct MaterialParamBool
    {
        PHX_DECLARE_REFLECT(MaterialParamFloat)

        std::string name;
        bool        value = false;
    };

    PHX_REFLECT(MaterialTechnique,
        PHX_FIELD(name),
        PHX_FIELD(vertex_shader),
        PHX_FIELD(pixel_shader),
    )

    struct MaterialParamTexture
    {
        PHX_DECLARE_REFLECT(MaterialParamTexture)

        std::string name;
        float       path;
    };

    struct ShaderGeneric
    {
        PHX_DECLARE_REFLECT(ShaderGeneric)
        std::string name;
        std::string value;
    };
    
    PHX_REFLECT(BlendStateDesc,
        PHX_FIELD(src_factor),
        PHX_FIELD(dst_factor),
        PHX_FIELD(blend_op),
    )

    struct ShaderDescriptor
    {
        PHX_DECLARE_REFLECT(ShaderDescriptor)
        std::string                  source;
        std::vector<ShaderGeneric>   generics;
    };

    struct MaterialArchetype
    {
        PHX_DECLARE_REFLECT(MaterialArchetype)

        ShaderDescriptor                    shader_desc;
        bool                                is_double_sided = false;
        ArchetypeRenderState                render_state;
        std::vector<MaterialTechnique>      techniques;
        std::vector<MaterialParamFloat>     params_float;
        std::vector<MaterialParamFloat2>    params_float2;
        std::vector<MaterialParamFloat3>    params_float3;
        std::vector<MaterialParamFloat4>    params_float4;
        std::vector<MaterialParamInt>       params_int;
        std::vector<MaterialParamBool>      params_bool;
        std::vector<MaterialParamTexture>   params_texture;
    };
    
    struct Material
    {
        PHX_DECLARE_REFLECT(Material)

        AssetPtr<MaterialArchetype>         archetype;
        std::vector<MaterialParamFloat>     params_float;
        std::vector<MaterialParamFloat2>    params_float2;
        std::vector<MaterialParamFloat3>    params_float3;
        std::vector<MaterialParamFloat4>    params_float4;
        std::vector<MaterialParamInt>       params_int;
        std::vector<MaterialParamBool>      params_bool;
        std::vector<MaterialParamTexture>   params_texture;
    };
    

    // -- This being being compiled out at this moment - consider deleteting.
    PHX_REFLECT(ArchetypeRenderState,
        PHX_FIELD(cull_mode),
        PHX_FIELD(front_face),
        PHX_FIELD(depth_test),
        PHX_FIELD(depth_write),
        PHX_FIELD(depth_compare),
        PHX_FIELD_NESTED(blend, BlendStateDesc),
    )

    PHX_REFLECT(MaterialParamFloat,
        PHX_FIELD(name),
        PHX_FIELD(value),
    )

    PHX_REFLECT(MaterialParamFloat2,
        PHX_FIELD(name),
        PHX_FIELD(value)
    )

    PHX_REFLECT(MaterialParamFloat3,
        PHX_FIELD(name),
        PHX_FIELD(value)
    )

    PHX_REFLECT(MaterialParamFloat4,
        PHX_FIELD(name),
        PHX_FIELD(value)
    )

    PHX_REFLECT(MaterialParamInt,
        PHX_FIELD(name),
        PHX_FIELD(value)
    )

    PHX_REFLECT(MaterialParamBool,
        PHX_FIELD(name),
        PHX_FIELD(value)
    )

    PHX_REFLECT(MaterialParamTexture,
        PHX_FIELD(name),
        PHX_FIELD(path)
    )

    PHX_REFLECT(ShaderGeneric,
        PHX_FIELD(name),
        PHX_FIELD(value),
    );

    PHX_REFLECT(ShaderDescriptor,
        PHX_FIELD(source),
        PHX_FIELD_ARRAY(generics, ShaderGeneric),
    );

    PHX_REFLECT(MaterialArchetype,
        PHX_FIELD_NESTED(shader_desc, ShaderDescriptor),
        PHX_FIELD(is_double_sided),
        PHX_FIELD_NESTED(render_state,      ArchetypeRenderState),
        PHX_FIELD_ARRAY(techniques,         MaterialTechnique),
        PHX_FIELD_ARRAY(params_float,       MaterialParamFloat),
        PHX_FIELD_ARRAY(params_float2,      MaterialParamFloat2),
        PHX_FIELD_ARRAY(params_float3,      MaterialParamFloat3),
        PHX_FIELD_ARRAY(params_float4,      MaterialParamFloat4),
        PHX_FIELD_ARRAY(params_int,         MaterialParamInt),
        PHX_FIELD_ARRAY(params_bool,        MaterialParamBool),
        PHX_FIELD_ARRAY(params_texture,     MaterialParamTexture)
    )

    PHX_REFLECT(Material,
        PHX_FIELD_ASSET(archetype,          MaterialArchetype),
        PHX_FIELD_ARRAY(params_float,       MaterialParamFloat),
        PHX_FIELD_ARRAY(params_float2,      MaterialParamFloat2),
        PHX_FIELD_ARRAY(params_float3,      MaterialParamFloat3),
        PHX_FIELD_ARRAY(params_float4,      MaterialParamFloat4),
        PHX_FIELD_ARRAY(params_int,         MaterialParamInt),
        PHX_FIELD_ARRAY(params_bool,        MaterialParamBool),
        PHX_FIELD_ARRAY(params_texture,     MaterialParamTexture)
    )
}