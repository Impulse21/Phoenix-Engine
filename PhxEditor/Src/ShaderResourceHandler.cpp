#include "ShaderResourceHandler.h"

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/RHI/PhxShaderCompiler.h>

using namespace PhxEngine;

PhxEngine::RefCountPtr<PhxEngine::Resource> PhxEditor::ShaderResourceHandler::Load(std::unique_ptr<PhxEngine::IBlob>&& fileData)
{


#if false
	std::string shaderPath = this->m_shaderPath + shaderFilename;
	std::filesystem::path absoluteFilePath = std::filesystem::absolute(shaderPath);

	// TODO: Add incremental build support
	// if (this->IsShaderOutdated(shaderFilename))
	{
		// Compile shader
		ShaderCompiler::CompileOptions options = {};
		options.Model = ->GetMinShaderModel();
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
#endif
	return nullptr;
}

void PhxEditor::ShaderResourceHandler::LoadDispatch(std::string const& path)
{
}
