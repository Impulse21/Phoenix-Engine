#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/VFS.h>

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

		virtual RefCountPtr<Resource> LoadFromPak(IFileSystem* fs, FileHandle handle) const = 0;
		virtual RefCountPtr<Resource> LoadLoose(IFileSystem* fs, FileHandle handle) const = 0;

	};
}