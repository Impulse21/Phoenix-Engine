#pragma once

#include <PhxData/Asset.h>
#include <PhxData/Any.h>
#include <string>
#include <unordered_map>

namespace phx::renderer
{

	struct MaterialAsset : public phx::data::Asset
	{
		PHX_DECLARE_ASSET(MaterialAsset);
        
        // Path to the shader resource this material uses.
        std::string shader_virtual_path;

        std::unordered_map<std::string, phx::data::Any> parameters;

        std::unordered_map<std::string, std::string> texture_paths;
	};
}