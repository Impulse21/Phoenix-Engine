#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>

#include <PhxResource/VFSResource.h>
#include <memory>
#include <filesystem>

namespace phx
{
	template<typename T>
	struct ResourceExtension;

	class IResource;
	class IResourceHandler
	{
	public:
		virtual RefCountPtr<IResource> Load(std::filesystem::path const& path, std::shared_ptr<IResourceFileSystem> const& fs) const = 0;
		virtual RefCountPtr<IResource> Load(std::filesystem::path const& path, std::shared_ptr<IResourceFileSystem> const& fs, FileHandle filehandle, size_t offset) const = 0;

		virtual ~IResourceHandler() = default;
	};
}