#include <PhxEngine/PhxEngine.h>
 
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/Memory.h>

#include <PhxEngine/Core/Initializer.h>

#include <taskflow/taskflow.hpp>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;

namespace GltfHelpers
{

	// glTF only support DDS images through the MSFT_texture_dds extension.
	// Since cgltf does not support this extension, we parse the custom extension string as json here.
	// See https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Vendor/MSFT_texture_dds 
	static const cgltf_image* ParseDDSImage(const cgltf_texture* texture, const cgltf_data* objects)
	{
		for (size_t i = 0; i < texture->extensions_count; i++)
		{
			const cgltf_extension& ext = texture->extensions[i];

			if (!ext.name || !ext.data)
			{
				continue;
			}

			if (strcmp(ext.name, "MSFT_texture_dds") != 0)
			{
				continue;
			}

			size_t extensionLength = strlen(ext.data);
			if (extensionLength > 1024)
			{
				return nullptr; // safeguard against weird inputs
			}

			jsmn_parser parser;
			jsmn_init(&parser);

			// count the tokens, normally there are 3
			int numTokens = jsmn_parse(&parser, ext.data, extensionLength, nullptr, 0);

			// allocate the tokens on the stack
			jsmntok_t* tokens = (jsmntok_t*)alloca(numTokens * sizeof(jsmntok_t));

			// reset the parser and prse
			jsmn_init(&parser);
			int numParsed = jsmn_parse(&parser, ext.data, extensionLength, tokens, numTokens);
			if (numParsed != numTokens)
			{
				// // LOG_CORE_WARN("Failed to parse the DDS glTF extension: %s", ext.data);
				return nullptr;
			}

			if (tokens[0].type != JSMN_OBJECT)
			{
				// // LOG_CORE_WARN("Failed to parse the DDS glTF extension: %s", ext.data);
				return nullptr;
			}

			for (int k = 1; k < numTokens; k++)
			{
				if (tokens[k].type != JSMN_STRING)
				{
					// // LOG_CORE_WARN("Failed to parse the DDS glTF extension: %s", ext.data);
					return nullptr;
				}

				if (cgltf_json_strcmp(tokens + k, (const uint8_t*)ext.data, "source") == 0)
				{
					++k;
					int index = cgltf_json_to_int(tokens + k, (const uint8_t*)ext.data);
					if (index < 0)
					{
						// // LOG_CORE_WARN("Failed to parse the DDS glTF extension: %s", ext.data);
						return nullptr;
					}

					if (size_t(index) >= objects->images_count)
					{
						// // LOG_CORE_WARN("Invalid image index %s specified in glTF texture definition", index);
						return nullptr;
					}

					cgltf_image* retVal = objects->images + index;

					// Code To catch mismatch in DDS textures
					// assert(std::filesystem::path(retVal->uri).replace_extension(".png").filename().string() == std::filesystem::path(texture->image->uri).filename().string());

					return retVal;
				}

				// this was something else - skip it
				k = cgltf_skip_json(tokens, k);
			}
		}

		return nullptr;
	}

	static std::pair<const uint8_t*, size_t> CgltfBufferAccessor(const cgltf_accessor* accessor, size_t defaultStride)
	{
		// TODO: sparse accessor support
		const cgltf_buffer_view* view = accessor->buffer_view;
		const uint8_t* Data = (uint8_t*)view->buffer->data + view->offset + accessor->offset;
		const size_t stride = view->stride ? view->stride : defaultStride;
		return std::make_pair(Data, stride);
	}

	struct CgltfContext
	{
		std::shared_ptr<IFileSystem> FileSystem;
		std::vector<std::shared_ptr<IBlob>> Blobs;
	};

	static cgltf_result CgltfReadFile(
		const struct cgltf_memory_options* memory_options,
		const struct cgltf_file_options* file_options,
		const char* path,
		cgltf_size* size,
		void** Data)
	{
		CgltfContext* context = (CgltfContext*)file_options->user_data;

		std::unique_ptr<IBlob> dataBlob = context->FileSystem->ReadFile(path);
		if (!dataBlob)
		{
			return cgltf_result_file_not_found;
		}

		if (size)
		{
			*size = dataBlob->Size();
		}

		if (Data)
		{
			*Data = (void*)dataBlob->Data();  // NOLINT(clang-diagnostic-cast-qual)
		}

		context->Blobs.push_back(std::move(dataBlob));

		return cgltf_result_success;
	}

	void CgltfReleaseFile(
		const struct cgltf_memory_options*,
		const struct cgltf_file_options*,
		void*)
	{
		// do nothing
	}
}

int main(int argc, const char** argv)
{
    Core::Log::Initialize();
    Core::MemoryService::GetInstance().Initialize({
            .MaximumDynamicSize = PhxGB(1)
        });

    PhxEngine::Core::CommandLineArgs args(argc, argv);

    // Mount the File System
    std::shared_ptr<IFileSystem> fileSystem = Core::CreateNativeFileSystem();

    std::string inputGltfFile;
    if (!args.GetString("inputFile", inputGltfFile))
    {
        LOG_ERROR("Missing required input File.");

		Core::MemoryService::GetInstance().Finalize();
        return -1;
    }

    std::filesystem::path gltfFile = inputGltfFile;
	/*
    if (gltfFile.extension() != L".gltf" || gltfFile.extension() != L".glb")
    {
        LOG_ERROR("Missing required input File.");

		Core::MemoryService::GetInstance().Finalize();
        return -1;
    }
	*/
    // Alright, lets load this data up


	LOG_INFO("Building Output.");
    std::unique_ptr<IBlob> blob = nullptr;
    tf::Taskflow flow;
    tf::Task loadFileTask = flow.emplace([&blob, &fileSystem, &gltfFile]() { 
		blob = fileSystem->ReadFile(gltfFile);
		}).name("Load File");


	GltfHelpers::CgltfContext cgltfContext =
	{
		.FileSystem = fileSystem,
		.Blobs = {}
	};

    cgltf_data* objects = nullptr;

	tf::Task stop = flow.emplace([]() {}).name("stop");

	tf::Task loadData = flow.emplace([](tf::Subflow& subflow) { // subflow task B
			tf::Task B1 = subflow.emplace([]() { LOG_INFO("Loading Meshes"); }).name("B1");
			tf::Task B2 = subflow.emplace([]() { LOG_INFO("Loading Materials"); }).name("B2");
			tf::Task B3 = subflow.emplace([]() { LOG_INFO("Loading Nodes"); }).name("B3");
			B3.succeed(B1, B2);  // B3 runs after B1 and B2
		});

    tf::Task parseObjects = flow.emplace([&blob, &objects, &cgltfContext]() {
            cgltf_options options = { };
            options.file.read = &GltfHelpers::CgltfReadFile;
            options.file.release = &GltfHelpers::CgltfReleaseFile;
            options.file.user_data = &cgltfContext;
			cgltf_result res = cgltf_parse(&options, blob->Data(), blob->Size(), &objects);

			if (res != cgltf_result_success)
			{
				LOG_ERROR("Coouldn't load glTF file");
				return false;
			}

			return true;
        });

	loadFileTask.precede(parseObjects);
	parseObjects.precede(stop, loadData);
	

	LOG_INFO("Waiting on executor.");
	tf::Executor executor;
	executor.run(flow).wait();

	LOG_INFO("Finised.");
	Core::MemoryService::GetInstance().Finalize();
    return 0;
}
