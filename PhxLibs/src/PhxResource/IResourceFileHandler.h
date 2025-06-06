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

		virtual RefCountPtr<Resource> LoadFromPak(data::IVirtualFileSystem* vfs) const = 0;
		virtual RefCountPtr<Resource> LoadLoose(data::IVirtualFileSystem* vfs) const = 0;

	};
}