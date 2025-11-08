#pragma once

#include <PhxCore/RefCountPtr.h>
#include <string>
#include <PhxCore/StringHash.h>

#define PHX_DEFINE_RES_FILE_EXT(TYPE, ext)																	\
	namespace phx {																							\
		template<> struct ResourceFileExtension<TYPE> {	static constexpr const char* value = #ext; };			\
		template<> struct ResourceFileHandlerId<TYPE> { static constexpr phx::StringHash value = #ext##_hash; };	\
	}

namespace phx
{
	template<typename T>
	struct ResourceFileExtension;

	template<typename T>
	struct ResourceFileHandlerId;

	class IVirtualFileSystem;
	class IIoQueue;
	struct AsyncResourceDescriptor;

	class ResourceSystem;
	struct Resource;

	struct LoadAsyncContext
	{
		std::string virtual_file_path;
		RefCountPtr<Resource> resource;
		ResourceSystem* resource_system;
		IVirtualFileSystem* vfs;
		IIoQueue* io_queue;
	};

	class ResourceFileHandler
	{
	public:
		virtual ~ResourceFileHandler() = default;

		virtual StringHash GetResourceTypeHash() const = 0;
		virtual bool IsStale(AsyncResourceDescriptor const& resource_descriptor, IVirtualFileSystem* vfs) const = 0;
		virtual RefCountPtr<Resource> CreatePlaceholder() const = 0;
		virtual void LoadAsync(IIoQueue* io_queue, RefCountPtr<Resource> resource, AsyncResourceDescriptor const& resource_descriptor) const = 0;

	};
}