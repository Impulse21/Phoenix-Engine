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
		float MetallicFactor = 1.0f;
		float Rougness = 1.0f;
		uint32_t flags;
    };

    // Used at load time to construct descriptor tables
    struct MaterialTextureData
    {
		uint16_t stringIdx[renderer::kNumTextures];
        uint32_t addressModes;
    };

	struct ModelData
	{
		std::vector<std::string> TextureNames;
		std::vector<MaterialConstantData> MaterialConstants;
		std::vector<MaterialTextureData> MaterialTextures;
	};

	class ModelImporter
	{
	public:
		virtual ~ModelImporter() = default;

		virtual bool Import(IFileSystem* fs, std::string const& fileName, ModelData& outModel) = 0;
	};
}