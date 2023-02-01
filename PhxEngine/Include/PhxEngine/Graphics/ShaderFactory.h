#pragma once

#include <unordered_map>
#include <filesystem>

#include "PhxEngine/RHI/PhxRHI.h"

namespace PhxEngine::Core
{
	class IFileSystem;
	class IBlob;
}

namespace PhxEngine::Graphics
{
	class ShaderFactory
	{
	public:
		ShaderFactory(
			RHI::IGraphicsDevice* graphicsDevice,
			std::shared_ptr<Core::IFileSystem> fs);
		~ShaderFactory() = default;

		RHI::ShaderHandle LoadShader(std::string const& filename, RHI::ShaderDesc const& shaderDesc);

	private:
		std::shared_ptr<Core::IBlob> GetByteCode(std::string const& filename);

		bool IsShaderOutdated(std::filesystem::path const& shader);

	private:
		std::shared_ptr<Core::IFileSystem> m_fs;
		std::unordered_map<std::string, std::weak_ptr<Core::IBlob>> m_bytecodeCache;
		RHI::IGraphicsDevice* m_graphicsDevice;
	};
}

