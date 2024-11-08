#pragma once
#include <cstdint>
#include <memory>

#include <string>
#include <vector>

#include "phxGfxDeviceResources.h"

namespace phx
{
	class IFileSystem;
}
namespace phx::gfx
{
	namespace ShaderCompiler
	{
		struct Input
		{
			union
			{
				struct
				{
					uint8_t DisableOptimization : 1;
					uint8_t StripReflection : 7;
				};
				uint8_t Flags = 0;
			};

			ShaderFormat Format = ShaderFormat::None;
			ShaderStage ShaderStage = ShaderStage::VS;
			ShaderModel ShaderModel = ShaderModel::SM_6_6;
			std::string SourceFilename;
			std::string EntryPoint = "main";
			std::vector<std::string> IncludeDir;
			std::vector<std::string> Defines;
			IFileSystem* FileSystem;
		};

		struct Output
		{
			std::shared_ptr<void> Internal;

			const uint8_t* ByteCode = nullptr;
			size_t ByteCodeSize = 0;

			std::vector<uint8_t> ShaderHash;
			std::string ErrorMessage;
			std::vector<std::string> Dependencies;
		};

		Output Compile(Input const& input);
	}
}
