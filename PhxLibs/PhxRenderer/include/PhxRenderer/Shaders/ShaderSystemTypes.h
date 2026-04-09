#pragma once

#include <PhxCore/Span.h>
#include <PhxCore/StringHash.h>

#include <PhxRhi/PhxRhi_Types.h>

#include <string>

namespace phx::renderer
{
    struct ShaderEntryPoint
    {
        std::string name;
        rhi::ShaderStage stage;
    };

    struct ShaderDescriptor
    {
        std::string virtual_path;

        struct GenericArg
        {
            std::string name;
            std::string value;

            bool is_type = false;
        };

        std::vector<GenericArg> generic_args;
        std::vector<ShaderEntryPoint> entry_points;

        Hash64 GetHash() const;
    };

    enum class ShaderStageFlags : uint16_t
    {
        None            = BIT(0),
        Vertex          = BIT(1),
        Pixel           = BIT(2),
        Compute         = BIT(3),
        Mesh            = BIT(4),
        Amplification   = BIT(5),
    };
    PHX_ENUM_CLASS_FLAGS(ShaderStageFlags)

    // TODO: Move to rendering pipeline .h
    struct PassInfo
    {
        std::string_view    name;
        ShaderStageFlags    active_stages;
    };

    // -- Reflection structs ---
    struct ShaderFieldDesc
    {
        StringHash name;
        uint32_t offset;
        uint32_t size;
    };

    struct ShaderStructDesc
    {
        StringHash name;
        uint32_t size;
        std::vector<ShaderFieldDesc> fields;
    };
}