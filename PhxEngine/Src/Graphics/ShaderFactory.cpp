#include "phxpch.h"
#include "PhxEngine/Graphics/ShaderFactory.h"

// #include "ShaderCompiler.h"
#include "PhxEngine/Core/Helpers.h"
#include <PhxEngine/Core/VirtualFileSystem.h>


using namespace PhxEngine::Graphics;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;

PhxEngine::Graphics::ShaderFactory::ShaderFactory(
	RHI::IGraphicsDevice* graphicsDevice,
	std::shared_ptr<IFileSystem> fs,
	std::filesystem::path const& basePath)
	: m_graphicsDevice(graphicsDevice)
	, m_fs(std::move(fs))
	, m_basePath(basePath)
{
}

ShaderHandle PhxEngine::Graphics::ShaderFactory::CreateShader(
	std::string const& filename,
	ShaderDesc const& shaderDesc)
{
	std::shared_ptr<IBlob> shaderByteCode = this->GetByteCode(filename);
	
	if (!shaderByteCode)
	{
		return {};
	}
	
	return this->m_graphicsDevice->CreateShader(
		shaderDesc,
		Core::Span(
			static_cast<const uint8_t*>(shaderByteCode->Data()),
			shaderByteCode->Size()));
}

/*
ShaderHandle PhxEngine::Graphics::ShaderFactory::LoadShader(ShaderStage stage, std::string const& shaderFilename)
{
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
}
	*/

std::shared_ptr<IBlob> PhxEngine::Graphics::ShaderFactory::GetByteCode(std::string const& filename)
{
	std::string adjustedName = filename;
	{
		size_t pos = adjustedName.find(".hlsl");
		if (pos != std::string::npos)
		{
			adjustedName.erase(pos, 5);
		}
	}

	std::filesystem::path shaderFilePath = this->m_basePath / (adjustedName + ".cso");

	std::weak_ptr<IBlob>& dataWkPtr = this->m_bytecodeCache[adjustedName];
	if (auto cachedData = dataWkPtr.lock())
	{
		return cachedData;
	}

	std::shared_ptr<IBlob> Data = this->m_fs->ReadFile(shaderFilePath);

	this->m_bytecodeCache[adjustedName] = std::weak_ptr(Data);

	return Data;
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
