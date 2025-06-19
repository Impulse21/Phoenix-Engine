#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>

namespace phx
{
	template<typename T>
	struct ResourceFileExtension;

	template<typename T>
	struct ResourceFileHandlerId;

	namespace data
	{
		class IVirtualFileSystem;
		class IStreamingManager;
	}

	class ResourceSystem;
	struct Resource;

	struct LoadAsyncContext
	{
		std::string virtual_file_path;
		RefCountPtr<Resource> resource;
		ResourceSystem* resource_system;
		data::IVirtualFileSystem* vfs;
		data::IStreamingManager* loader;
	};

	class ResourceFileHandler
	{
	public:
		virtual ~ResourceFileHandler() = default;

		virtual StringHash GetResourceTypeHash() const = 0;
		virtual void LoadAsync(ResourceSystem* resource_system, RefCountPtr<Resource> asset, std::string const& virtual_file_path) const = 0;

	};
}