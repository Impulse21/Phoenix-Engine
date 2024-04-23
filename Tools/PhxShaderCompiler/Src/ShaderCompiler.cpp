
#include <thread>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <PhxEngine/Core/Logger.h>
#include <PhxEngine/RHI/PhxShaderCompiler.h>


namespace fs = std::filesystem;

using namespace PhxEngine::RHI;

namespace
{
	YAML::Node gOptions;
	std::vector<std::string> gIncludeDirs;
	fs::file_time_type gConfigWriteTime; 

	struct CompileTask
	{
		std::string SourceFile;
		std::string ShaderName;
		std::string EntryPoint;
		std::string CombinedDefines;
		std::string CommandLine;
		ShaderCompiler::CompilerInput Input;
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
		ShaderCompiler::CompilerInput input;
		input.Filename = shaderCompile["file_path"].as<std::string>();
		input.ShaderType = ShaderType::HLSL6;
		input.ShaderStage = ParseShaderStage(shaderCompile["stage"].as<std::string>());
		input.ShaderModel = ShaderModel::SM_6_7;
		input.EntryPoint = "main";
		input.IncludeDirs = gIncludeDirs;
		if (!gOptions["debug"].IsNull() && gOptions["debug"].as<bool>())
			input.Flags |= ShaderCompiler::CompilerFlags::DisableOptimization | ShaderCompiler::CompilerFlags::EmbedDebug;

		
		return true;
	}
}



int main(int argc, char** argv)
{
	PhxEngine::Logger::Startup();
	gOptions= YAML::Load(argv[1]);

	std::string inputFile = gOptions["input_file"].as<std::string>();
	if (inputFile.empty() || !fs::exists(inputFile))
		throw std::runtime_error("unable to find input file");

	std::string outputPath = gOptions["output_path"].as<std::string>();
	bool packShaders = gOptions["pack_shaders"].as<bool>();

	auto includeDirNode = gOptions["include_dir"];
	if (!includeDirNode.IsNull() && includeDirNode.IsSequence())
	{
		gIncludeDirs.reserve(includeDirNode.size());
		// Iterate over each element of the sequence
		for (std::size_t i = 0; i < includeDirNode.size(); ++i)
		{
			// Push each element into the vector
			gIncludeDirs.push_back(includeDirNode[i].as<std::string>());
		}
	}

	// If we have updated this binary, we should recompile everything
	gConfigWriteTime = fs::last_write_time(inputFile);
	gConfigWriteTime = std::max(gConfigWriteTime, fs::last_write_time(argv[0]));


	std::ifstream configFile(inputFile);
	YAML::Node configNode = YAML::Load(configFile);

	const YAML::Node& shaders = configNode["shaders"];
	for (YAML::const_iterator it = shaders.begin(); it != shaders.end(); ++it)
	{
		const YAML::Node& shaderCompile = *it;
		if (!ProcessShaderConfig(shaderCompile))
			return 1;
	}


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
