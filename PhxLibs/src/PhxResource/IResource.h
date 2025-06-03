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
		std::atomic_uint8_t State = (uint8_t)~0;

		enum State : uint8_t
		{
			Loaded = 0,
			Loading = 0x0F,
			Error = 0x7F,
			Unloaded = 0xFF

		};

		bool IsLoaded()
		{
			return State == State::Loaded;
		}
	};

}