#pragma once

#include <string>

#include "PhxEngine/Graphics/RHI/PhxRHI.h"

namespace PhxEngine::RHI
{
	namespace ShaderCompiler
	{
		struct CompileOptions
		{
			RHI::ShaderType Type = RHI::ShaderType::None;
			RHI::ShaderStage Stage = RHI::ShaderStage::None;

			RHI::ShaderModel Model = RHI::ShaderModel::SM_6_0;
			std::string EntryPoint = "main";
			std::vector<std::string> IncludeDirectories;
			std::vector<std::string> Defines;
		};

		struct CompileResult
		{
			bool Successful = false;

			std::shared_ptr<void> _InternalData;
			std::string ErrorMsg;

			const uint8_t* ShaderData = nullptr;
			size_t ShaderSize = 0;

			std::vector<uint8_t> ShaderPDB;
			std::vector<uint8_t> ShaderHash;
			std::vector<std::string> Dependencies;

			bool HasErrors() const { return this->ErrorMsg.empty(); }
			bool HasPDBSymbols() const { return !this->ShaderPDB.empty(); }
			bool HasShaderHash() const { return !this->ShaderHash.empty(); }
		};

		extern CompileResult CompileShader(std::string const& shaderSourceFile, CompileOptions const& compileOptions);
	}
}

