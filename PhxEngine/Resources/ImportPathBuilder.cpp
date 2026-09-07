#include "ImportPathBuilder.h"

#include <PhxEngine/Core/PathUtils.h>

namespace phx::ImportPathBuilder
{
    std::string ForPrefab(const std::string& source_path)
    {
        
        std::string dir = GetDirectory(source_path);
        std::string filename = GetFileNameWithoutExt(source_path);
        std::string cache_dir = JoinPaths(dir, ".cache/prefabs/");

        // 3. Assemble the final path with the new extension.
        return JoinPaths(cache_dir, filename + ".phxfab");
    }

    std::string ForMesh(const std::string& source_path, const std::string& sub_asset_name)
    {
        std::string dir = GetDirectory(source_path);
        std::string source_filename = GetFileNameWithoutExt(source_path);
        std::string cache_dir = JoinPaths(dir, ".cache/meshes/");

		// TODO: Use the extenion type from resource
		const char* extension = ".phxmsh";
        std::string new_filename = source_filename + "_" + sub_asset_name + extension;

        return JoinPaths(cache_dir, new_filename);
    }

	std::string ForTexture(const std::string& source_path, const std::string& sub_asset_name)
	{
		std::string dir = GetDirectory(source_path);
		std::string source_filename = GetFileNameWithoutExt(source_path);
		std::string cache_dir = JoinPaths(dir, ".cache/textures/");

		const char* extension = ".phxtex";
		std::string new_filename = source_filename + "_" + sub_asset_name + extension;

		return JoinPaths(cache_dir, new_filename);
	}

	std::string ForMaterial(const std::string& source_path, const std::string& sub_asset_name)
	{
		std::string dir = GetDirectory(source_path);
		std::string source_filename = GetFileNameWithoutExt(source_path);
		std::string cache_dir = JoinPaths(dir, ".cache/material/");

		const char* extension = ".phxast";// ResourceTraits<renderer::MaterialResource>::Extension;
		std::string new_filename = source_filename + "_" + sub_asset_name + extension;

		return JoinPaths(cache_dir, new_filename);
	}
}
