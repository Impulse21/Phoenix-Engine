#include <PhxCore/Base.h>
#include <PhxCore/StringUtils.h>
#include <PhxCore/IO/FileUtils.h>

#include <PhxEngine/JobSystem.h>
#include <nlohmann/json.hpp>

#include "ModelImporterFactory.h"
#include "ModelExporter.h"

#include <PhxCore/Application.h>
using namespace phx;


// TOOD: Fix this up, these are just stubs for now, but asset compilers shouldn't need to do this.

namespace phx
{
	IApplication* CreateApplication()
	{
		return nullptr;
	}

	void DeleteApplication(IApplication* ptr)
	{
		delete ptr;
	}
}


int wmain(int argc, wchar_t** argv)
{
	if (argc != 0 && wcscmp(argv[1], L"-version") == 0)
	{
		std::cout << "0.0.1" << std::endl;
		return 0;
	}

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
		phx::Result<ModelData> model_data = model_compiler->Import(input_file, {});

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