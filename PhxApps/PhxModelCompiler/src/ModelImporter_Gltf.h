#pragma once

#include "IModelImporter.h"

class GltfModelImporter : public IModelImporter
{
public:
	phx::Result<ModelData>  Import(std::string const& file) override;
};

