#define NOMINMAX 

#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/Logger.h>
#include <PhxEngine/Core/StopWatch.h>
#include <PhxEngine/Core/RefCountPtr.h>
#include <PhxEngine/Renderer/Drawable.h>

#include <PhxEngine/Core/Memory.h>

#include "GltfAssetImporter.h"
#include "MeshOptimizationPipeline.h"
#include "MeshletGenerationPipeline.h"
#include "MeshCompiler.h"
#include "MeshResourceExporter.h"

#include <json.hpp>

#include <dstorage.h>
#include <fstream>

using namespace PhxEngine;
using namespace nlohmann;

#define ASSERT_SUCCEEDED( hr, ... )				\
        if (FAILED(hr)) {						\
            PHX_LOG_ERROR("HRESULT failed");	\
            __debugbreak(); \
        }

// "{ \"input\" : \"C:\\Users\\dipao\\source\\repos\\Impulse21\\Phoenix-Engine\\Assets\\Main.1_Sponza\\NewSponza_Main_glTF_002.gltf\", \"output_dir\": \"C:\\Users\\dipao\\source\\repos\\Impulse21\\Phoenix-Engine\\Assets\\Main.1_Sponza_Baked\" }"
// "{ \"input\" : \"..\\..\\..\\Assets\\Box.gltf\", \"output_dir\": \"..\\..\\..\\Output\\resources\",  \"compression\" : \"None\" }"
int main(int argc, const char** argv)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hr));

	Logger::Startup();
	if (argc == 0)
	{
		PHX_LOG_ERROR("Input json is expected");
		return -1;
	}

	json inputSettings = json::parse(argv[1]);

	const std::string inputTag = "input";
	const std::string outputTag = "output_dir";
	const std::string compressionTag = "compression";
	if (!inputSettings.contains(inputTag))
	{
		PHX_LOG_ERROR("Input is required");
		return -1;
	}

	if (!inputSettings.contains(outputTag))
	{
		PHX_LOG_ERROR("Out directory");
		return -1;
	}

	const std::string& gltfInput = inputSettings[inputTag];
	const std::string& outputDirectory = inputSettings[outputTag];

	Compression compression = Compression::None;
	if (inputSettings.contains(compressionTag))
	{
		const std::string& compressionStr = inputSettings[compressionTag];
		if (compressionStr == "gdeflate")
			compression = Compression::GDeflate;
	}

	PHX_LOG_INFO("Baking Assets from %s to %s", gltfInput.c_str(), outputDirectory.c_str());
	std::unique_ptr<IFileSystem> fileSystem = FileSystemFactory::CreateNativeFileSystem();

	StopWatch elapsedTime;
	Pipeline::ImportedObjects importedObjects = {};
	{
		PHX_LOG_INFO("Importing GLTF File: %s", gltfInput.c_str());
		elapsedTime.Begin();
		Pipeline::GltfAssetImporter gltfImporter;
		gltfImporter.Import(fileSystem.get(), gltfInput, importedObjects);

		PHX_LOG_INFO("Importing GLTF File took %f seconds", elapsedTime.Elapsed().GetSeconds());
	}

	//  tf::Taskflow texturePipelineTask = taskflow.emplace([&importedObjects](tf::sub))

	StackAllocator tempAllocator(PhxGB(1));

	Pipeline::MeshOptimizationPipeline optmizationPipeline;
	for (auto& mesh : importedObjects.Meshes)
	{
		optmizationPipeline.Optimize(tempAllocator, mesh);
	}

	elapsedTime.Begin();
	Pipeline::MeshletGenerationPipeline pipeline(64, 124);
	for (auto& mesh : importedObjects.Meshes)
	{
		pipeline.GenerateMeshletData(mesh);
	}

	// Create and save Drawable.
	
	// Get the buffer compression interface for DSTORAGE_COMPRESSION_FORMAT_GDEFLATE
	constexpr uint32_t NumCompressionThreads = 6;
	RefCountPtr<IDStorageCompressionCodec> bufferCompression;
	ASSERT_SUCCEEDED(
		DStorageCreateCompressionCodec(DSTORAGE_COMPRESSION_FORMAT_GDEFLATE, NumCompressionThreads, IID_PPV_ARGS(&bufferCompression)));


	// Create Drawable
	for (auto& mesh : importedObjects.Meshes)
	{

		std::filesystem::path destPath(outputDirectory);
		destPath /= mesh.Name + ".pmesh";
		destPath.make_preferred();

		PHX_LOG_INFO("Saving out Mesh Resources '%s'", destPath.generic_string().c_str());

		std::ofstream outStream(destPath, std::ios::out | std::ios::trunc | std::ios::binary);

		Pipeline::MeshResourceExporter::Export(outStream, compression, mesh);
	}

    return 0;
}
