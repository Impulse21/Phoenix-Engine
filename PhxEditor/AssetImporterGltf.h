#pragma once

#include <string>

namespace phx::editor
{
	class AssetImporterGltf
	{
	public:
		static bool Import(std::string const& filename)
		{
			AssetImporterGltf importer(filename);

			return importer.ImporterInternal();
		}

	private:
		AssetImporterGltf(std::string const& filename)
		{

		}

		bool ImporterInternal();
	};
}

