#pragma once

#include <PhxCore/RefCountPtr.h>

namespace phx
{
	class IResource
	{
	public:
		virtual ~IResource() = default;

		virtual bool IsLoaded() const = 0;

	public:
		virtual unsigned long AddRef() = 0;
		virtual unsigned long Release() = 0;

        // Non-copyable and non-movable
        // IResource(const IResource&) = delete;
        // IResource(const IResource&&) = delete;
        // IResource& operator=(const IResource&) = delete;
        // IResource& operator=(const IResource&&) = delete;
	};

	struct Resource : public RefCounted
	{
		uint32_t State = 0;

		bool IsLoaded()
		{
			return State == 0;
		}
	};

}