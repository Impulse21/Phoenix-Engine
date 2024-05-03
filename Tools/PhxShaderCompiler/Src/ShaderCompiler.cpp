
#include <thread>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <stdexcept>
#include <csignal>
#include <PhxEngine/Core/Logger.h>
#include <PhxEngine/RHI/PhxShaderCompiler.h>
#include <PhxEngine/Core/CommandLineArgs.h>
#include <PhxEngine/Core/BinaryBuilder.h>
#include <PhxEngine/Resource/ResourceFileFormat.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <cmdparser.hpp>


namespace fs = std::filesystem;

using namespace PhxEngine;
using namespace PhxEngine::RHI;

namespace
{
	std::string gInputFile;
	std::string gOutputPath;
	std::unique_ptr<IFileSystem> gOutputFS;
	std::vector<std::string> gIncludeDirs;
	fs::file_time_type gConfigWriteTime;

	std::mutex gReportMutex;
	std::mutex gTaskMutex;
	bool gTerminate = false;

	struct CompileTask
	{
		ShaderCompiler::CompilerInput Input;
		fs::path OutputFile;
	};

	std::vector<CompileTask> gCompileTasks;

	ShaderStage ParseShaderStage(std::string_view str)
	{
		if (str == "vs")
			return ShaderStage::Vertex;
		if (str == "ps")
			return ShaderStage::Pixel;
		if (str == "cs")
			return ShaderStage::Compute;
		if (str == "mas")
			return ShaderStage::Amplification;
		if (str == "ms")
			return ShaderStage::Mesh;

		return ShaderStage::Vertex;
	}

	bool ProcessShaderConfig(const YAML::Node& shaderCompile)
	{
		std::string filepath = shaderCompile["file_path"].as<std::string>();
		fs::path compiledShaderName = filepath.substr(0, filepath.find_last_of('.')) + ".pso";

		fs::path sourceFile = fs::path(gInputFile).parent_path() / shaderCompile["file_path"].as<std::string>();

		CompileTask& task = gCompileTasks.emplace_back();

		ShaderCompiler::CompilerInput& input = task.Input;
		input.Filename = sourceFile.generic_string();
		input.ShaderType = ShaderType::HLSL6;
		input.ShaderStage = ParseShaderStage(shaderCompile["stage"].as<std::string>());
		input.ShaderModel = ShaderModel::SM_6_7;
		input.EntryPoint = "main";
		input.IncludeDirs = gIncludeDirs;
		if (!CommandLineArgs::HasArg("debug"))
			input.Flags |= ShaderCompiler::CompilerFlags::DisableOptimization | ShaderCompiler::CompilerFlags::EmbedDebug;

		task.OutputFile = compiledShaderName;
		return true;
	}

	void SignalHandler(int sig)
	{
		(void)sig;
		gTerminate = true;

		std::scoped_lock guard(gReportMutex);
		PHX_LOG_CORE_ERROR("SIGINT received, terminating");
	}

}

void CompileThreadProc()
{
	while (!gTerminate)
	{
		CompileTask task;
		{
			std::scoped_lock guard(gTaskMutex);
			if (gCompileTasks.empty())
				return;

			task = gCompileTasks[gCompileTasks.size() - 1];
			gCompileTasks.pop_back();
		}

		ShaderCompiler::CompilerResult result = ShaderCompiler::Compile(task.Input);
		if (!result.InternalResource)
		{
			std::stringstream ss;
			ss << "Compiling shader " << task.Input.Filename << " Messages \n";
			ss << "\t" << result.ErrorMessage;
			PHX_LOG_ERROR("'%s'", ss.str().c_str());
		}

		BinaryBuilder binaryBuilder;
		size_t headerOffset = binaryBuilder.Reserve<ShaderFileFormat::Header>();
		size_t metadataHeaderOffset = binaryBuilder.Reserve<ShaderFileFormat::MetadataHeader>();
		size_t dataOffset = binaryBuilder.Reserve<uint8_t>(result.ShaderData.Size());

		binaryBuilder.Commit();

		// -- Set data ---
		auto* dataPtr = binaryBuilder.Place<uint8_t>(dataOffset);
		std::memcpy(dataPtr, result.ShaderData.begin(), result.ShaderData.Size());
		
		// -- Set metadata ---
		auto* metadataHeader = binaryBuilder.Place<ShaderFileFormat::MetadataHeader>(metadataHeaderOffset);
		metadataHeader->ShaderStage = task.Input.ShaderStage;

		// -- Fill in header ---
		auto header = binaryBuilder.Place<ShaderFileFormat::Header>(headerOffset);
		header->ID = ShaderFileFormat::ResourceId;
		header->Version = ShaderFileFormat::CurrentVersion;
		header->MetadataHeader.UncompressedSize = sizeof(ShaderFileFormat::MetadataHeader);
		header->MetadataHeader.CompressedSize = header->MetadataHeader.UncompressedSize;
		header->MetadataHeader.Offset = metadataHeaderOffset;
		header->ShaderData.Offset = dataOffset;

		gOutputFS->WriteFile(task.OutputFile, binaryBuilder.GetSpan());
	}
}


int main(int argc, char** argv)
{
	PhxEngine::Logger::Startup();
	cli::Parser parser(argc, argv);

	// TODO: I am here;
	parser.set_required<std::string>("inFile", "inFile", "Input Configuration File.");
	parser.set_required<std::string>("outputDir", "outputDir", "Output directory.");
	parser.set_optional<std::string>("includeDir", "includeDir", "", "Include Directory.");
	parser.set_optional<bool>("parallel", "parallel", false, "Compile shaders on seperate threads.");
	parser.set_optional<bool>("pack", "pack", false, "Pack shaders into a pack file.");
	parser.set_optional<bool>("debug", "debug", false, "Builds Debug with debug flags");
	parser.run_and_exit_if_error();


	gInputFile = parser.get<std::string>("inFile");
	if (!fs::exists(gInputFile))
	{
		PHX_LOG_ERROR("Unable to locate config file %s", gInputFile.c_str());
		return -1;
	}

	gOutputPath = parser.get<std::string>("outputDir");

	bool packShaders = parser.get<bool>("pack");

	std::filesystem::create_directories(gOutputPath);

	std::shared_ptr<IFileSystem> nativeFS = FileSystemFactory::CreateNativeFileSystem();
	gOutputFS = FileSystemFactory::CreateRelativeFileSystem(nativeFS, gOutputPath);

	std::string includerDirs = parser.get<std::string>("includeDir");
	if (!includerDirs.empty())
	{
		std::istringstream f(includerDirs);
		std::string val;
		while (std::getline(f, val, ';'))
		{
			gIncludeDirs.push_back(val);
		}
	}

	gIncludeDirs.push_back(fs::path(gInputFile).parent_path().generic_string());

	// If we have updated this binary, we should recompile everything
	gConfigWriteTime = fs::last_write_time(gInputFile);
	gConfigWriteTime = std::max(gConfigWriteTime, fs::last_write_time(argv[0]));


	std::ifstream configFile(gInputFile);
	YAML::Node configNode = YAML::Load(configFile);

	const YAML::Node& shaders = configNode["shaders"];
	for (YAML::const_iterator it = shaders.begin(); it != shaders.end(); ++it)
	{
		const YAML::Node& shaderCompile = *it;
		if (!ProcessShaderConfig(shaderCompile))
			return 1;
	}


	unsigned int threadCount = std::thread::hardware_concurrency();
	if (threadCount == 0 || !parser.get<bool>("parallel"))
	{
		threadCount = 1;
	}

	std::signal(SIGINT, SignalHandler);

	std::vector<std::thread> threads;
	threads.resize(threadCount);
	for (unsigned int threadIndex = 0; threadIndex < threadCount; threadIndex++)
	{
		threads[threadIndex] = std::thread(CompileThreadProc);
	}
	for (unsigned int threadIndex = 0; threadIndex < threadCount; threadIndex++)
	{
		threads[threadIndex].join();
	}


	// Execite

	PhxEngine::Logger::Shutdown();
#if false
	if (!g_Options.parse(argc, argv))
	{
		cout << g_Options.errorMessage << endl;
		return 1;
	}

	switch (g_Options.platform)
	{
	case Platform::DXBC: g_PlatformName = "DXBC"; break;
	case Platform::DXIL: g_PlatformName = "DXIL"; break;
	case Platform::SPIRV: g_PlatformName = "SPIR-V"; break;
	case Platform::UNKNOWN: g_PlatformName = "UNKNOWN"; break; // never happens
	}

	for (const auto& fileName : g_Options.ignoreFileNames)
	{
		g_IgnoreIncludes.push_back(fileName);
	}

	g_ConfigWriteTime = fs::last_write_time(g_Options.inputFile);

	// Updated shaderCompiler executable also means everything must be recompiled
	g_ConfigWriteTime = std::max(g_ConfigWriteTime, fs::last_write_time(argv[0]));

	ifstream configFile(g_Options.inputFile);
	uint32_t lineno = 0;
	for (string line; getline(configFile, line);)
	{
		lineno++;

		if (!trim(line))
			continue;

		if (!expandPermutations(lineno, line))
			return 1;
	}

	if (g_CompileTasks.empty())
	{
		cout << "All " << g_PlatformName << " outputs are up to date." << endl;
		return 0;
	}

	g_OriginalTaskCount = (int)g_CompileTasks.size();
	g_ProcessedTaskCount = 0;

	{
		// Workaround for weird behavior of _popen / cmd.exe on Windows
		// with quotes around the executable name and also around some other arguments.

		static char envBuf[1024]; // use a static array because putenv uses it by reference
#ifdef WIN32
		snprintf(envBuf, sizeof(envBuf), "COMPILER=\"%s\"", g_Options.compilerPath.c_str());
#else
		snprintf(envBuf, sizeof(envBuf), "COMPILER=%s", g_Options.compilerPath.c_str());
#endif
		putenv(envBuf);

		if (g_Options.verbose)
		{
			cout << envBuf << endl;
		}
	}

	unsigned int threadCount = thread::hardware_concurrency();
	if (threadCount == 0 || !g_Options.parallel)
	{
		threadCount = 1;
	}

	signal(SIGINT, signal_handler);

	vector<thread> threads;
	threads.resize(threadCount);
	for (unsigned int threadIndex = 0; threadIndex < threadCount; threadIndex++)
	{
		threads[threadIndex] = thread(compileThreadProc);
	}
	for (unsigned int threadIndex = 0; threadIndex < threadCount; threadIndex++)
	{
		threads[threadIndex].join();
	}

	if (!g_CompileSuccess || g_Terminate)
		return 1;

	for (const pair<const string, vector<BlobEntry>>& it : g_ShaderBlobs)
	{
		if (!WriteShaderBlob(it.first, it.second))
			return 1;
	}

	return 0;
#endif
	return 0;
}
