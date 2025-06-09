#pragma once

#include <PhxCore/RefCountPtr.h>

namespace phx
{
	struct Resource : public RefCounted
	{
		enum State : uint8_t
		{
			Loaded = 0,
			Loading = 0x0F,
			Error = 0x7F,
			Unloaded = 0xFF
		};

		std::atomic_uint8_t State = State::Unloaded;

		bool IsLoaded()
		{
			return State == State::Loaded;
		}
	};

}