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
		class IAsyncIOSystem;
	}

	class ResourceSystem;
	struct Resource;

	struct LoadAsyncContext
	{
		const char* virtual_file_path;
		RefCountPtr<Resource> resource;
		ResourceSystem* resource_system;
		data::IVirtualFileSystem* vfs;
		data::IAsyncIOSystem* loader;
	};

	class ResourceFileHandler
	{
	public:
		virtual ~ResourceFileHandler() = default;

		virtual StringHash GetResourceTypeHash() const = 0;
		virtual void LoadAsync(LoadAsyncContext const& context) const = 0;

	};
}