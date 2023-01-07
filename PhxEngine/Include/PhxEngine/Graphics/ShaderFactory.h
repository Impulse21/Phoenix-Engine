#pragma once

#include <unordered_map>
#include <filesystem>

#include "PhxEngine/RHI/PhxRHI.h"

namespace PhxEngine::Graphics
{
	class ShaderFactory
	{
	public:
		ShaderFactory(
			RHI::IGraphicsDevice* graphicsDevice,
			std::string const& shaderPath,
			std::string const& shaderSourcePath);
		~ShaderFactory() = default;

		RHI::ShaderHandle LoadShader(RHI::ShaderStage stage, std::string const& shaderFilename);

	private:
		bool IsShaderOutdated(std::filesystem::path const& shader);

	private:
		const std::string m_shaderPath;
		const std::string m_shaderSourcePath;
		RHI::IGraphicsDevice* m_graphicsDevice;
	};
}

