#pragma once

#include <PhxEngine/Core/RefCountPtr.h>
#include <dstorage.h>

namespace PhxEngine::Core
{

	class DStorageLoader
	{
	public:
		static void Initialize(bool disableGpuDecompression = false);
		static void Finalize();

		IDStorageFactory* GetFactory() { return m_factory.Get(); }
		IDStorageQueue1* GetMemoryQeueu() { return m_memoryQueue.Get();}
		IDStorageQueue1* GetGpuQeueu() { return m_GpuQueue.Get(); }

	private:
		inline static Core::RefCountPtr<IDStorageFactory> m_factory;
		inline static Core::RefCountPtr<IDStorageQueue1> m_memoryQueue;
		inline static Core::RefCountPtr<IDStorageQueue1> m_GpuQueue;
	};
}

