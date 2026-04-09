#include <iostream>

#include <PhxCore/Span.h>

#include <PhxCore/Log.h>
#include <PhxCore/TaskScheduler.h>

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Platform/Platform.h>
#include <PhxCore/IO/MemoryRegion.h>

#include <PhxResourceCompiler/GltfPrefabCooker.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace phx;

struct ResourceConfig 
{
    std::string input_gltf;
    std::string output_dir;
    bool rebuild = false;
};

void PrintUsage(const char* exec_name) 
{
    std::stringstream ss;
    ss << "Usage: " << exec_name << " --input <file.gltf> --output <dir>\n"
              << "Options:\n"
              << "  -i, --input   Path to the source .gltf file\n"
              << "  -o, --output  Directory for compiled resources\n"
              << "  -r, --rebuild Flag indicating whether to rebuild resources\n"
              << "  -h, --help    Display this help message\n";

    PHX_INFO(ss.str().c_str());
}

bool ParseArgs(int argc, char* argv[], bool& containedError, ResourceConfig& config) 
{
    phx::Span<const char*> args(argv + 1, argc - 1);
    
    containedError = false;
    for (size_t i = 0; i < args.size(); ++i) 
    {
        std::string_view arg = args[i];

        if (arg == "-h" || arg == "--help") 
        {
            PrintUsage(argv[0]);
            return false;
        } 
        else if (arg == "-i" || arg == "--input") 
        {
            if (i + 1 < args.size()) 
            {
                config.input_gltf = args[++i];
            } 
            else 
            {
                PHX_ERROR("Error: --input requires a file path.");
                containedError = true;
                return false;
            }
        } 
        else if (arg == "-o" || arg == "--output") 
        {
            if (i + 1 < args.size()) 
            {
                config.output_dir = args[++i];
            } else 
            {
                containedError = true;
                PHX_ERROR("Error: --output requires a directory path.");
                return false;
            }
        }
        else if (arg == "-r" || arg == "--rebuild") 
        {
            config.rebuild = true;
        }
    }
    
    return true;
}

phx::Result<phx::MemoryBuffer> LoadFileIntoMemory(const char* input_path, phx::PlatformFileAttributes& out_file_attr)
{
    phx::Result<phx::PlatformFileAttributes> fileAttributeResult = phx::Platform::GetFileAttr(input_path);
    if (fileAttributeResult.HasError())
    {
        PHX_ERROR("Failed to get file attributes for input GLTF file: %s", input_path);
        return phx::Unexpected(phx::ResultError::Failure);
    }

    out_file_attr = fileAttributeResult.GetValue();
    phx::MemoryBuffer file_buffer(out_file_attr.size);

    phx::Result<PlatformFileHandle> file_handle_result = phx::Platform::OpenFile(input_path, phx::FileMode::Read);
    if (!file_handle_result.HasError())
    {
        PHX_ERROR("Failed to open input GLTF file: %s", input_path);
        return phx::Unexpected(phx::ResultError::Failure);
    }

    PlatformFileHandle file_handle = file_handle_result.GetValue();

    phx::Platform::ReadFile(file_handle, file_buffer.Data(), file_buffer.Size());
    phx::Platform::CloseFile(file_handle);

    return file_buffer;
}

int main(int argc, char* argv[])
{
    phx::Log::Initialize();
    if (argc == 1)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    ResourceConfig config;
    bool containedError = false;
    if (!ParseArgs(argc, argv, containedError, config))
    {
        return containedError ? 1 : 0;
    }

    phx::TaskScheduler::Initialize();
    phx::TaskScheduler::InitializeCorePool();
    // Parse command line arguments and determine which cooker to run.

    // Validation
    if (config.input_gltf.empty() || config.output_dir.empty()) 
    {
        PHX_ERROR("Error: Missing required arguments.");
        PrintUsage(argv[0]);
        return 1;
    }

    // Proceed to Resource Compilation
    PHX_INFO("Starting resource compilation...");
    PHX_INFO("Input GLTF: %s", config.input_gltf.c_str());
    PHX_INFO("Output Directory: %s", config.output_dir.c_str());   

    // Construct virtual file system.
    phx::PlatformFileAttributes out_file_attr;
    phx::Result<phx::MemoryBuffer> file_data_result = LoadFileIntoMemory(config.input_gltf.c_str(), out_file_attr);
    if (file_data_result.HasError())
    {
        PHX_ERROR("Failed to load input GLTF file: %s", config.input_gltf.c_str());
        return 1;
    }
    
    phx::MemoryBuffer& file_data = file_data_result.GetValue();

    cgltf_options options = {
#if false
        .file = {
            .read = &CgltfReadFile,
            .release = &CgltfReleaseFile,
        }
#endif
    };

    cgltf_data* gltf_data = nullptr;
    cgltf_result result = cgltf_parse(&options, file_data.Data(), file_data.Size(), &gltf_data);
    if (result != cgltf_result_success)
    {
        PHX_ERROR("Couldn't parse glTF file '{0}'", config.input_gltf.c_str());
        return 1;
    }

    result = cgltf_load_buffers(&options, gltf_data, config.input_gltf.c_str());
    if (result != cgltf_result_success)
    {
        PHX_ERROR(
            "Couldn't load glTF `{0}` Binary data '{1}'"
            , config.input_gltf.c_str()
            , static_cast<uint32_t>(result));

        return 1;
    }


	const bool success = 
        phx::resource::compiler::CGltfPrefabCooker::Cook(
            *gltf_data,
            config.output_dir.c_str(),
            out_file_attr,
            config.rebuild);

    if (!success)
    {
        PHX_ERROR("Failed to cook glTF prefab.");
        return 1;
    }

    return 0;
}
