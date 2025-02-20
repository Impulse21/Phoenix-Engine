#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxResource/VFSResource.h>

namespace phx
{
	class IResource
	{
	public:
		virtual ~IResource() = default;

		virtual bool IsLoaded() const = 0;
		virtual FileHandle GetFileHandle() const = 0;

	public:
		virtual unsigned long AddRef() = 0;
		virtual unsigned long Release() = 0;

        // Non-copyable and non-movable
        // IResource(const IResource&) = delete;
        // IResource(const IResource&&) = delete;
        // IResource& operator=(const IResource&) = delete;
        // IResource& operator=(const IResource&&) = delete;
	};

}