#include <iostream>

#include <PhxCore/Span.h>

#include <PhxCore/Log.h>
#include <PhxCore/TaskScheduler.h>

#include <PhxCore/IO/FileSystems.h>
#include <PhxCore/IO/MemoryRegion.h>
#include <PhxCore/Platform/Platform.h>

#include <PhxResourceCompiler/Mesh/MeshCompiler.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

using namespace phx;

enum class PipelineType
{
    Unknown = 0,
    Mesh,
    Prefab,
    Level,
};

enum class UpVector
{
    Z_Positive = 0,
    Y_Positive,
};

struct ResourceConfig 
{
    std::string input_gltf;
    std::string content_root;
    PipelineType pipeline_type = PipelineType::Unknown;
    UpVector resource_up_vector = UpVector::Z_Positive;
    bool rebuild = false;
};

struct GltfLoadResult
{
    cgltf_data*                  data      = nullptr;
    phx::PlatformFileAttributes  file_attr = {};
};

void PrintUsage(const char* exec_name) 
{
    std::stringstream ss;
    ss << "Usage: " << exec_name << " --input <file.gltf> --output <dir>\n"
              << "Options:\n"
              << "  -i, --input         Path to the source .gltf file\n"
              << "  -c, --content_root  Directory for compiled resources\n"
              << "  -r, --rebuild       Flag indicating whether to rebuild resources\n"
              << "  -t, --type          String to identify the pipeline type for this compile\n"
              << "  -a, --axis          Define src axis (default=z+ as blender)\n"
              << "  -h, --help          Display this help message\n";

    PHX_INFO(ss.str().c_str());
}


const char* PipelineTypeName(PipelineType type)
{
    switch (type)
    {
        case PipelineType::Mesh:
           return "Mesh";
        case PipelineType::Prefab:
            return "Prefab";
        case PipelineType::Level:
            return "Level";
        default:
            return "Unknown";
    }
}

PipelineType ParsePipelineType(std::string_view s)
{
    if (s == "mesh")
        return PipelineType::Mesh;
    if (s == "prefab") 
        return PipelineType::Prefab;
    if (s == "level") 
        return PipelineType::Level;

    return PipelineType::Unknown;
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
        else if (arg == "-c" || arg == "--content_root") 
        {
            if (i + 1 < args.size()) 
            {
                config.content_root = args[++i];
            } 
            else 
            {
                containedError = true;
                PHX_ERROR("Error: --content_root requires a directory path.");
                return false;
            }
        }
        else if (arg == "-r" || arg == "--rebuild") 
        {
            config.rebuild = true;
        }
        else if (arg == "-t" || arg == "--type")
        {
            if (i + 1 < args.size())
            {
                config.pipeline_type = ParsePipelineType(args[++i]);
            }
            else
            {
                containedError = true;
                PHX_ERROR("Error: --type requires a type input (mesh|prefab|level).");
                return false;
            }
        }
    }
    
    return true;
}

phx::Result<std::unique_ptr<phx::IBlob>> LoadFileIntoMemory(const char* input_path, phx::PlatformFileAttributes& out_file_attr)
{
    phx::Result<phx::PlatformFileAttributes> fileAttributeResult = phx::Platform::GetFileAttr(input_path);
    if (fileAttributeResult.HasError())
    {
        PHX_ERROR("Failed to get file attributes for input GLTF file: %s", input_path);
        return phx::Unexpected(phx::ResultError::Failure);
    }

    out_file_attr = fileAttributeResult.GetValue();
    std::unique_ptr<phx::Blob> file_blob = std::make_unique<phx::Blob>(out_file_attr.size);


    Result<PlatformFileHandle> gltf_file_result =  Platform::OpenFile(input_path, FileMode::Read);
    
    if (!gltf_file_result)
    {
        PHX_ERROR("Failed to open GLTF file {0}", input_path);
        return phx::Unexpected(phx::ResultError::NotFound);
    }
    
    const size_t actual_read = phx::Platform::ReadFile(gltf_file_result.GetValue(), file_blob->Data(), file_blob->Size());

    PHX_ASSERT(actual_read == file_blob->Size());

    Platform::CloseFile(gltf_file_result.GetValue());

    return std::move(file_blob);
}

phx::Result<GltfLoadResult> LoadGltf(const ResourceConfig& config)
{
    GltfLoadResult load_results = {};
    
    phx::Result<std::unique_ptr<phx::IBlob>> file_data_result = LoadFileIntoMemory(config.input_gltf.c_str(), load_results.file_attr);
    if (file_data_result.HasError())
    {
        PHX_ERROR("Failed to load input GLTF file: %s", config.input_gltf.c_str());
        return phx::Unexpected(phx::ResultError::Failure);
    }
    
    std::unique_ptr<phx::IBlob>& file_data = file_data_result.GetValue();
    cgltf_options options = {
#if false
        .file = {
            .read = &CgltfReadFile,
            .release = &CgltfReleaseFile,
        }
#endif
    };

    cgltf_result result = cgltf_parse(&options, file_data->Data(), file_data->Size(), &load_results.data);
    if (result != cgltf_result_success)
    {
        PHX_ERROR("Couldn't parse glTF file '{0}'", config.input_gltf.c_str());
        return phx::Unexpected(phx::ResultError::Failure);
    }

    result = cgltf_load_buffers(&options, load_results.data, config.input_gltf.c_str());
    if (result != cgltf_result_success)
    {
        PHX_ERROR(
            "Couldn't load glTF `{0}` Binary data '{1}'"
            , config.input_gltf.c_str()
            , static_cast<uint32_t>(result));

        return phx::Unexpected(phx::ResultError::Failure);
    }

    return load_results;
}

static void ExecuteMeshPipeline(Span<cgltf_mesh> meshes, Span<cgltf_material> materials);

int main(int argc, char* argv[])
{
    phx::Log::Initialize();
    if (argc == 1)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    ResourceConfig config;
    bool contained_error = false;
    if (!ParseArgs(argc, argv, contained_error, config))
    {
        return contained_error ? 1 : 0;
    }


    PHX_INFO("-----------------------------------");
    PHX_INFO("Pipeline : %s",  PipelineTypeName(config.pipeline_type));
    PHX_INFO("Input    : %s",  config.input_gltf.c_str());
    PHX_INFO("Output   : %s",  config.content_root.c_str());
    PHX_INFO("Rebuild  : %s",  config.rebuild ? "yes" : "no");
    PHX_INFO("-----------------------------------");

    phx::TaskScheduler::Initialize();
    phx::TaskScheduler::InitializeCorePool();
    // Parse command line arguments and determine which cooker to run.

    // Validation
    if (config.input_gltf.empty() || config.content_root.empty()) 
    {
        PHX_ERROR("Error: Missing required arguments.");
        PrintUsage(argv[0]);
        return 1;
    }

    // -- Detect pipeline ----
    
    // Proceed to Resource Compilation

    std::string src_path = phx::GetDirectory(config.input_gltf);
    std::string content_root_path = phx::GetDirectory(config.content_root);

    phx::Result<GltfLoadResult> load_result = LoadGltf(config);
    if (!load_result)
    {
        cgltf_free(load_result->data);
        // -- Error handled in above functions
        return 1;
    }

    switch (config.pipeline_type)
    {
    case PipelineType::Mesh:
    {
        // Mesh Cooker
        PHX_INFO("Running Mesh pipeline");
        Span<cgltf_mesh> meshes(load_result->data->meshes, load_result->data->meshes_count);
        Span<cgltf_material> materials(load_result->data->materials, load_result->data->materials_count);   
        ExecuteMeshPipeline(meshes, materials);

        break;
    }
    case PipelineType::Prefab:
        // Prefab cooker
        PHX_WARN("Prefab pipeline is not implemented yet. Exiting.");
        break;
    case PipelineType::Level:
        // Level Cooker
        PHX_WARN("Level pipeline is not implemented yet. Exiting.");
        break;
    }

	const bool success = true;

    cgltf_free(load_result->data);
    if (!success)
    {
        PHX_ERROR("Failed to cook glTF prefab.");
        return 1;
    }

    return 0;
}

static void ExecuteMeshPipeline(Span<cgltf_mesh> meshes, Span<cgltf_material> materials)
{
    constexpr uint32_t group_size = 1; // TODO: Tune this based on the workload and system capabilities.
    
    TaskScheduler::Dispatch([meshes, materials](const DispatchId& dispatch_id) 
    {
        const cgltf_mesh& mesh = meshes[dispatch_id.global_index];
        PHX_INFO("Cooking mesh {0}/{1}: {2}", dispatch_id.global_index + 1, meshes.size(), mesh.name ? mesh.name : "Unnamed Mesh");
        Result<MemoryBuffer> compile_result = phx::resource::compiler::CompileMesh(mesh, materials);

    },meshes.size(), group_size, TaskScheduler::InitializeCorePool());

    TaskScheduler::Wait(TaskScheduler::InitializeCorePool());
}