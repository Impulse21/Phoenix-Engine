#include <PhxCore/Base.h>
#include <PhxCore/StringUtils.h>
#include <PhxCore/IO/FileUtils.h>

#include <PhxEngine/JobSystem.h>
#include <nlohmann/json.hpp>

#include "ModelImporterFactory.h"
#include "ModelExporter.h"

using namespace phx;


int wmain(int argc, wchar_t** argv)
{
	phx::Log::Initialize();
	if (argc == 0)
	{
		PHX_INFO("JSON config expected");
		return -1;
	}

	phx::JobSystem::Initialize();

	std::wstring w_config_file = argv[1];
	std::string config_file;
	StringConvert(w_config_file, config_file);

	nlohmann::json config = nlohmann::json::parse(config_file);

	const std::string& input_file = config["input_file"];
	const std::string& output_file = config["output_file"];

	std::string extension = GetFileExt(input_file);

	if (ModelImporterFactory::IsSupported(extension))
	{
		std::unique_ptr<IModelImporter> model_compiler = ModelImporterFactory::Create(extension);
		phx::Result<ModelData> model_data = model_compiler->Import(input_file);

		if (model_data)
		{
			std::ofstream out_stream(output_file, std::ios::out | std::ios::trunc | std::ios::binary);
			ExportOptions options = {};

			ModelExporter::Export(out_stream, model_data.GetValue(), options);
		}
		else
		{
			PHX_ERROR("Failed to import model data");
		}
	}
	else
	{
		PHX_ERROR("Unsupported input file type {0}. Currently only support gltf", extension);
	}

	phx::JobSystem::Shutdown();

}