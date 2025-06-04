#pragma once

#include <PhxData/AsyncIODefinitions.h>
namespace phx::data
{
	class IAsyncIOSystem
	{
	public:
		inline static IAsyncIOSystem* Ptr = nullptr;
	public:
		virtual ~IAsyncIOSystem() = default;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual void QueueRead(AsyncReadRequest&& request) = 0;

		virtual void Tick(float delta_time) = 0;
	};
}