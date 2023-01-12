#include "phxpch.h"
#include "PhxEngine/Graphics/ShaderFactory.h"

// #include "ShaderCompiler.h"
#include "PhxEngine/Core/Helpers.h"

using namespace PhxEngine::Graphics;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;

PhxEngine::Graphics::ShaderFactory::ShaderFactory(
	RHI::IGraphicsDevice* graphicsDevice,
	std::string const& shaderPath,
	std::string const& shaderSourcePath)
	: m_shaderPath(shaderPath)
	, m_graphicsDevice(graphicsDevice)
	, m_shaderSourcePath(shaderSourcePath)
{
}

ShaderHandle PhxEngine::Graphics::ShaderFactory::LoadShader(ShaderStage stage, std::string const& shaderFilename)
{
	/*
	std::string shaderPath = this->m_shaderPath + shaderFilename;
	std::filesystem::path absoluteFilePath = std::filesystem::absolute(shaderPath);

	// TODO: Add incremental build support
	// if (this->IsShaderOutdated(shaderFilename))
	{
		// Compile shader
		ShaderCompiler::CompileOptions options = {};
		options.Model = this->m_graphicsDevice->GetMinShaderModel();
		options.Type = this->m_graphicsDevice->GetShaderType();
		options.Stage = stage;

		// Construct Source 
		std::string sourcedir = this->m_shaderSourcePath;
		std::filesystem::path absoluteSourceFilePath = std::filesystem::absolute(sourcedir);
		options.IncludeDirectories.push_back(absoluteSourceFilePath.string());

		absoluteSourceFilePath /= shaderFilename;
		absoluteSourceFilePath.replace_extension(".hlsl");

		if (this->m_graphicsDevice->GetMinShaderModel() >= RHI::ShaderModel::SM_6_6)
		{
			options.Defines.push_back("USE_RESOURCE_HEAP");
		}

		auto output = ShaderCompiler::CompileShader(absoluteSourceFilePath.string(), options);

		if (!output.Successful)
		{
			throw std::runtime_error("Failed to compile shader");
		}

		Helpers::DirectoryCreate(absoluteFilePath.root_directory());
		Helpers::FileWrite(absoluteFilePath.string(), output.ShaderData, output.ShaderSize);

		if (output.HasPDBSymbols())
		{
			auto symbolsPath = absoluteFilePath;
			symbolsPath.replace_extension(".pdb");
			Helpers::FileWrite(symbolsPath.string(), output.ShaderPDB.data(), output.ShaderPDB.size());
		}
	}

	std::vector<uint8_t> buffer;
	if (!Helpers::FileRead(absoluteFilePath.string(), buffer))
	{
		assert(false);
		return nullptr;
	}

	ShaderDesc desc;
	desc.DebugName = absoluteFilePath.filename().string();
	desc.Stage = stage;

	return this->m_graphicsDevice->CreateShader(desc, buffer.data(), buffer.size());
	*/
	return nullptr;
}

bool PhxEngine::Graphics::ShaderFactory::IsShaderOutdated(std::filesystem::path const& shaderFilename)
{
	std::filesystem::path absoluteFilePath = std::filesystem::absolute(shaderFilename);
	if (!std::filesystem::exists(absoluteFilePath))
	{
		return true;
	}

	std::filesystem::path shaderBuildMetadataPath = absoluteFilePath;
	shaderBuildMetadataPath.replace_extension(".meta");
	if (!std::filesystem::exists(shaderBuildMetadataPath))
	{
		return true;
	}

	// auto lastWriteTime = std::filesystem::last_write_time(absoluteFilePath);
	// TODO: Need to write out metadata to know we don't need to compile shaders every time we start up.
	return false;
}
