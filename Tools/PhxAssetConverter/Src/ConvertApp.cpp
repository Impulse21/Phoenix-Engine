#define NOMINMAX 

#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/StringHashTable.h>
#include <PhxEngine/Core/Log.h>

#include "GltfAssetImporter.h"

using namespace PhxEngine;

int main(int argc, const char** argv)
{
    Core::Log::Initialize();
	Core::CommandLineArgs::Initialize();
	std::string gltfInput;
	Core::CommandLineArgs::GetString("input", gltfInput);


	std::string outputDirectory;
	Core::CommandLineArgs::GetString("output_dir", outputDirectory);

	PHX_LOG_INFO("Baking Assets from %s to %s", gltfInput.c_str(), outputDirectory.c_str());
	Core::StringHashTable::Import(StringHashTableFilePath);
	std::unique_ptr<Core::IFileSystem> fileSystem = Core::CreateNativeFileSystem();
	
	Pipeline::ImportedObjects importedObjects = {};
	{
		Pipeline::GltfAssetImporter gltfImporter;
		gltfImporter.Import(fileSystem.get(), gltfInput, importedObjects);
	}

    return 0;
}
