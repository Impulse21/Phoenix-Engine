#include <iostream>

#include <PhxCore/Span.h>

#include <PhxCore/Log.h>
#include <PhxCore/TaskScheduler.h>

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Platform/Platform.h>

struct ResourceConfig 
{
    std::string input_gltf;
    std::string output_dir;
};

void PrintUsage(const char* exec_name) 
{
    std::cout << "Usage: " << exec_name << " --input <file.gltf> --output <dir>\n"
              << "Options:\n"
              << "  -i, --input   Path to the source .gltf file\n"
              << "  -o, --output  Directory for compiled resources\n"
              << "  -h, --help    Display this help message\n";
}

int main(int argc, char* argv[])
{
    ResourceConfig config;
    phx::Span<std::string_view> args(argv + 1, argv + argc);
    phx::Log::Initialize();
    phx::TaskScheduler::Initialize();
    phx::TaskScheduler::InitializeCorePool();

    // Parse command line arguments and determine which cooker to run.
    

    return 0;
}

int main(int argc, char* argv[]) {
    ResourceConfig config;
    std::vector<std::string_view> args(argv + 1, argv + argc);

    for (size_t i = 0; i < args.size(); ++i) {
        std::string_view arg = args[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } 
        else if (arg == "-i" || arg == "--input") {
            if (i + 1 < args.size()) {
                config.input_gltf = args[++i];
            } else {
                std::cerr << "Error: --input requires a file path.\n";
                return 1;
            }
        } 
        else if (arg == "-o" || arg == "--output") {
            if (i + 1 < args.size()) {
                config.output_dir = args[++i];
            } else {
                std::cerr << "Error: --output requires a directory path.\n";
                return 1;
            }
        }
    }

    // Validation
    if (config.input_gltf.empty() || config.output_dir.empty()) {
        std::cerr << "Error: Missing required arguments.\n";
        print_usage(argv[0]);
        return 1;
    }

    // Proceed to Resource Compilation
    std::cout << "Compiling: " << config.input_gltf << "\n";
    std::cout << "Exporting to: " << config.output_dir << "\n";

    return 0;
}