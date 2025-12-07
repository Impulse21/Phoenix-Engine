#pragma once

#include <string>
#include <array>

#include <PhxCore/Result.h>

#include "ModelBuildData.h"

namespace phx::compiler
{
	struct ImportOptions
	{
		bool bake_transforms = false;
	};

	class IModelImporter
	{
	public:
		virtual ~IModelImporter() = default;

		virtual phx::Result<ModelData> Import(std::string const& file_name, ImportOptions const& import_options) = 0;
	};
}