#pragma once

namespace phx
{
	struct AsyncReadRequest;
	class IAsyncIOSystem
	{
	public:
		inline static IAsyncIOSystem* Ptr = nullptr;
	public:
		~IAsyncIOSystem() = default;

		virtual bool Initialize() = 0;

		virtual void Shutdown() = 0;
		virtual void QueueRead(AsyncReadRequest&& request) = 0;
	};


}