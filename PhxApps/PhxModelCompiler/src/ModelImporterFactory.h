#pragma once

#include <string>
#include <memory>

#include "IModelImporter.h"

namespace ModelImporterFactory
{
	bool IsSupported(std::string const& extension);
	std::unique_ptr<IModelImporter> Create(std::string const& extension);
}

