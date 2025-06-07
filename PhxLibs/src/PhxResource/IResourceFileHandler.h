#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxData/IVirtualFileSystem.h>

namespace phx
{
	template<typename T>
	struct ResourceFileExtension;

	template<typename T>
	struct ResourceFileHandlerId;


	struct Resource;
	class IResourceFileHandler
	{
	public:
		virtual ~IResourceFileHandler() = default;

		virtual RefCountPtr<Resource> LoadAsync(data::IVirtualFileSystem* vfs, const char* virtual_file_path) const = 0;

	};
}