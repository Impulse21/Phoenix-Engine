#pragma once

#include <string>
#include <array>

#include <PhxCore/Result.h>

#include "ModelBuildData.h"

class IModelImporter
{
public:
	virtual ~IModelImporter() = default;

	virtual phx::Result<ModelData> Import(std::string const& file_name) = 0;
};