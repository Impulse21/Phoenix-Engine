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
			std::shared_ptr<Core::IFileSystem> fs,
			std::filesystem::path const& basePath);
		~ShaderFactory() = default;

		RHI::ShaderHandle CreateShader(std::string const& filename, RHI::ShaderDesc const& shaderDesc);

	private:
		std::shared_ptr<Core::IBlob> GetByteCode(std::string const& filename);

		bool IsShaderOutdated(std::filesystem::path const& shader);

	private:
		std::mutex m_loadMutex;
		const std::filesystem::path m_basePath;
		std::shared_ptr<Core::IFileSystem> m_fs;
		std::unordered_map<std::string, std::weak_ptr<Core::IBlob>> m_bytecodeCache;
		RHI::IGraphicsDevice* m_graphicsDevice;
	};
}

