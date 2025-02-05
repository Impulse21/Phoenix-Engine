#pragma once

#include "MeshData.h"
#include <iostream>

namespace phx
{
	class PakFileExporter
	{
	public:
		PakFileExporter(const char* filename);

		PakFileExporter& PackMeshes(std::vector<MeshData> const& meshData);

	private:
	};
}

