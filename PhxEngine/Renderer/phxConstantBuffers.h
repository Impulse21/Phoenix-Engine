#pragma once
#include <phxBaseInclude.h>

namespace phx::renderer
{
    enum { kBaseColor, kMetallicRoughness, kOcclusion, kEmissive, kNormal, kNumTextures };
	STRUCT_ALIGN(256) struct MaterialConstants
	{
		float BaseColour[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float EmissiveColour[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float MetallicFactor = 1.0f;
		float Rougness = 1.0f;
        union
        {
            uint32_t flags;
            struct
            {
                // UV0 or UV1 for each texture
                uint32_t baseColorUV : 1;
                uint32_t metallicRoughnessUV : 1;
                uint32_t occlusionUV : 1;
                uint32_t emissiveUV : 1;
                uint32_t normalUV : 1;

                // Three special modes
                uint32_t twoSided : 1;
                uint32_t alphaTest : 1;
                uint32_t alphaBlend : 1;

                uint32_t _pad : 8;

                uint32_t alphaRef : 16; // half float
            };
        };
	};
}