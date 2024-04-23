#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <string>

namespace PhxEngine::RHI::ShaderCompiler
{
	namespace CompilerFlags
	{
		enum
		{
			None = 0,
			DisableOptimization = 1 << 0,
			EmbedDebug = 1 << 2,
			CreateHeaderFile = 1 << 3,
		};
	}
	struct CompilerInput
	{
		std::string Filename;
		uint32_t Flags = CompilerFlags::None;

		RHI::ShaderType ShaderType = ShaderType::HLSL6;
		RHI::ShaderModel ShaderModel = ShaderModel::SM_6_6;
		RHI::ShaderStage ShaderStage = ShaderStage::Vertex;
		PhxEngine::Span<std::string> Defines;
		PhxEngine::Span<std::string> IncludeDirs;
		std::string EntryPoint = "main";
	};

	struct CompilerResult
	{
		std::string ErrorMessage;
		std::vector<std::string> Dependencies;
		PhxEngine::Span<uint8_t> ShaderData;
		PhxEngine::Span<uint8_t> ShaderSymbols;

		std::vector<uint8_t> ShaderHash;

		std::shared_ptr<void> InternalResource;
		std::shared_ptr<void> InternalResourceSymbols;
	};

	CompilerResult Compile(CompilerInput const& input);
}

