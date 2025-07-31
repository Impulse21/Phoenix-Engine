#pragma once

#include "IModelImporter.h"


struct cgltf_data;


class GltfModelImporter : public IModelImporter
{
public:
	phx::Result<ModelData> Import(std::string const& file) override;

private:
	bool ImportMaterials(cgltf_data* gltf_data, ModelData& model_data);
	bool ImportMeshes(cgltf_data* gltf_data, ModelData& model_data);
};

