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

bool parse_args(Span<wchar_t*> args, std::string& input_file, std::string& output_file)
{
	// Start at 1 to skip the program name (argv[0])
	for (size_t i = 0; i < args.size(); i++)
	{
		std::wstring arg = args[i];
		if (arg == L"-i")
		{
			if (i + 1 < args.size())
			{
				std::wstring input_file_w = args[i + 1];
				StringConvert(input_file_w, input_file);
				i++;
			}
			else 
			{
				PHX_ERROR("Error: -i flag requires a value.");
				return false;
			}
		}
		else if (arg == L"-o") 
		{
			if (i + 1 < args.size())
			{
				std::wstring output_file_w = args[i + 1];
				StringConvert(output_file_w, output_file);
				i++;
			}
			else
			{
				PHX_ERROR("Error: -o flag requires a value.");
				return false;
			}
		}
	}

	return !input_file.empty() && !output_file.empty();
}

int wmain(int argc, wchar_t** argv)
{
	if (argc != 0 && wcscmp(argv[1], L"-version") == 0)
	{
		std::cout << "0.0.1" << std::endl;
		return 0;
	}

	phx::Log::Initialize();
	std::string input_file;
	std::string output_file;

	if (!parse_args(Span(argv, argc), input_file, output_file))
	{
		PHX_ERROR("Usage: {0} -i <input_file> -o <output_file>");
		return -1;
	}

	// Old config style - might switch back to this to allow processing multi payloads.
#if false
	std::wstring w_config_file = argv[1];
	std::string config_file;
	StringConvert(w_config_file, config_file);

	nlohmann::json config = nlohmann::json::parse(config_file);

	const std::string& input_file = config["input_file"];
	const std::string& output_file = config["output_file"];
#endif

	// TODO: Set up virtual file system
	std::string extension = GetFileExt(input_file);

	if (ModelImporterFactory::IsSupported(extension))
	{
		phx::JobSystem::Initialize();

		std::unique_ptr<IModelImporter> model_compiler = ModelImporterFactory::Create(extension);
		phx::Result<ModelData> model_data = model_compiler->Import(input_file, {});

		if (model_data)
		{
			std::ofstream out_stream(output_file, std::ios::out | std::ios::trunc | std::ios::binary);
			if (!out_stream.is_open())
			{
				PHX_ERROR("Failed to create file output file '{0}'", output_file.c_str());
				return -1;
			}
			ExportOptions options = {};

			ModelExporter::Export(out_stream, model_data.GetValue(), options);

			PHX_INFO("Save Model Resrouce to {0}", output_file.c_str());
			// save external data.
			// Export Material data
			
			// Export Manifiest file.
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