#pragma once

#include <string>
#include <vector>
#include <hlsl++.h>

namespace phx::resources
{
	namespace PSOFlags
	{
		enum : uint16_t
		{
			kAlphaBlend = PHX_BIT(1),
			kAlphaTest  = PHX_BIT(2),
			kTwoSided   = PHX_BIT(3),
		};
	}

    struct IntermediateMesh
    {
        IntermediateMesh() = default;
        PHX_MOVE_ONLY(IntermediateMesh);

        struct Primitive
        {
            std::vector<hlslpp::float3> positions;
            std::vector<hlslpp::float3> normals;
            std::vector<hlslpp::float2> texCoords_0;
            std::vector<hlslpp::float2> texCoords_1;
            std::vector<hlslpp::float4> tangents;
            std::vector<hlslpp::float3> colour;
            std::vector<hlslpp::uint4> joints_0;
            std::vector<hlslpp::float4> weights_0;

		    std::vector<u32> indices;

            std::string material_name;

            union
            {
                uint32_t hash;
                struct {
                    uint32_t pso_flags : 16;
                    uint32_t index_32 : 1;
                    uint32_t material_index : 15;
                };
            };
        };

        std::string name;
        std::vector<Primitive> primitives;
    };
}