#pragma once

#include <string>
#include <memory>

namespace PhxEngine
{
	class IResourceFileHanlder
	{
	public:
		virtual ~IResourceFileHanlder() = default;

		virtual std::string_view GetFileExtension() const = 0;
		virtual void RegisterResourceFile(std::string_view file) = 0;
	};

	namespace ResourceManager
	{
		void Startup();
		void Shutdown();

		void RegisterResourceHandler(std::unique_ptr<IResourceFileHanlder>&& resourceHandler);
		void MountPath(std::string const& path);
		void RegisterPath(std::string const& directory);
		void RegisterPack(std::string const& filename);

	}
}

