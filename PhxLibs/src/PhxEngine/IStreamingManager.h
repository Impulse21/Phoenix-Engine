#pragma once

#include <PhxEngine/StreamingDefintions.h>
#include <PhxCore/Span.h>

namespace phx
{
	class IStreamingManager
	{
	public:
		inline static IStreamingManager* Ptr = nullptr;
	public:
		virtual ~IStreamingManager() = default;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual void Submit(StreamingRequest&& request) = 0;

		virtual void Tick(float delta_time) = 0;
	};
}