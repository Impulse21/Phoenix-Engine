// PhxArchive.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

#include <dstorage.h>
#include <fstream>
#include <assert.h>

#include <Core/phxLog.h>
#include <Core/phxStopWatch.h>
#include <Core/phxVirtualFileSystem.h>

#include "3rdParty/nlohmann/json.hpp"
#include "phxModelImporter.h"
#include "phxModelImporterGltf.h"


using namespace phx;
using namespace phx::core;

using namespace nlohmann;

// "{ \"input\" : \"C:\\Users\\dipao\\source\\repos\\Impulse21\\Phoenix-Engine\\Assets\\Main.1_Sponza\\NewSponza_Main_glTF_002.gltf\", \"output_dir\": \"C:\\Users\\dipao\\source\\repos\\Impulse21\\Phoenix-Engine\\Assets\\Main.1_Sponza_Baked\" }"
// "{ \"input\" : \"..\\..\\..\\Assets\\Box.gltf\", \"output_dir\": \"..\\..\\..\\Output\\resources\",  \"compression\" : \"None\" }"
int main(int argc, const char** argv)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hr));

	Log::Initialize();
	if (argc == 0)
	{
		PHX_INFO("Input json is expected");
		return -1;
	}

	json inputSettings = json::parse(argv[1]);

	const std::string inputTag = "input";
	const std::string outputTag = "output_dir";
	const std::string compressionTag = "compression";
	if (!inputSettings.contains(inputTag))
	{
		PHX_ERROR("Input is required");
		return -1;
	}

	if (!inputSettings.contains(outputTag))
	{
		PHX_ERROR("Out directory");
		return -1;
	}

	const std::string& gltfInput = inputSettings[inputTag];
	const std::string& outputDirectory = inputSettings[outputTag];

#if false
	Compression compression = Compression::None;
	if (inputSettings.contains(compressionTag))
	{
		const std::string& compressionStr = inputSettings[compressionTag];
		if (compressionStr == "gdeflate")
			compression = Compression::GDeflate;
	}
#endif

	PHX_INFO("Creating PhxArchive '%s' from '%s'", outputDirectory.c_str(), gltfInput.c_str());
	std::unique_ptr<IFileSystem> fs = FileSystemFactory::CreateNativeFileSystem();

	phxModelImporterGltf gltfImporter(fs.get());
	// Import Model from GLTF
	phx::StopWatch elapsedTime;

	ModelData model = {};
	gltfImporter.Import(gltfInput, model);
	PHX_INFO("Importing GLTF File took %f seconds", elapsedTime.Elapsed().GetSeconds());


}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
