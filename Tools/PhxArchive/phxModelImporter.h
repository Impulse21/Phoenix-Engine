#pragma once

#include <string>
#include <vector>

#include <Renderer/phxConstantBuffers.h>

namespace phx
{
	class IFileSystem;

    // Unaligned mirror of MaterialConstants
    struct MaterialConstantData
    {
		float BaseColour[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		float EmissiveColour[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		float Metalness = 1.0f;
		float Rougness = 1.0f;
        union
        {
            uint32_t Flags;
            struct
            {
                // UV0 or UV1 for each texture
                uint32_t BaseColorUV : 1;
                uint32_t MetallicRoughnessUV : 1;
                uint32_t OcclusionUV : 1;
                uint32_t EmissiveUV : 1;
                uint32_t NormalUV : 1;

                // Three special modes
                uint32_t TwoSided : 1;
                uint32_t AlphaTest : 1;
                uint32_t AlphaBlend : 1;

                uint32_t _pad : 8;

                uint32_t AlphaCutoff : 16; // half float
            };
        };
    };

    // Used at load time to construct descriptor tables
    struct MaterialTextureData
    {
		uint16_t StringIdx[renderer::kNumTextures];
        uint32_t AddressModes;
    };

	struct ModelData
	{
		std::vector<std::string> TextureNames;
		std::vector<MaterialConstantData> MaterialConstants;
		std::vector<MaterialTextureData> MaterialTextures;
        std::vector<uint8_t> TextureOptions;
	};

	class ModelImporter
	{
	public:
		virtual ~ModelImporter() = default;

		virtual bool Import(IFileSystem* fs, std::string const& fileName, ModelData& outModel) = 0;
	};
}