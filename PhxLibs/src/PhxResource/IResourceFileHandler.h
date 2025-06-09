#pragma once

#include <PhxCore/RefCountPtr.h>

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

	struct Resource;
	class ResourceFileHandler
	{
	public:
		virtual ~ResourceFileHandler() = default;

		virtual RefCountPtr<Resource> LoadAsync(data::IVirtualFileSystem* vfs, data::IAsyncIOSystem* loader, const char* virtual_file_path) const = 0;

	};
}