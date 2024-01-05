#include <PhxEngine/Core/DStorageLoader.h>

#include <assert.h>
void PhxEngine::Core::DStorageLoader::Initialize(bool disableGpuDecompression)
{
    DSTORAGE_CONFIGURATION config{};
    config.DisableGpuDecompression = disableGpuDecompression;
    assert(DStorageSetConfiguration(&config));

    assert(DStorageGetFactory(IID_PPV_ARGS(&m_factory)));
    m_factory->SetDebugFlags(DSTORAGE_DEBUG_BREAK_ON_ERROR | DSTORAGE_DEBUG_SHOW_ERRORS);
    m_factory->SetStagingBufferSize(256 * 1024 * 1024);

    // Create the system memory queue, used for reading data into system memory.
    {
        DSTORAGE_QUEUE_DESC queueDesc{};
        queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
        queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
        queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        queueDesc.Name = "DStorageLoader::m_memoryQueue";

        assert(m_factory->CreateQueue(&queueDesc, IID_PPV_ARGS(&m_memoryQueue)));
    }

    // Create the GPU queue, used for reading GPU resources.
    {
        DSTORAGE_QUEUE_DESC queueDesc{};
        // queueDesc.Device = Graphics::g_Device;
        queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
        queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
        queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        queueDesc.Name = "g_dsGpuQueue";

        assert(m_factory->CreateQueue(&queueDesc, IID_PPV_ARGS(&m_GpuQueue)));
    }

    // Configure custom decompression queue
#if false
    assert(g_dsFactory.As(&g_customDecompressionQueue));
    g_customDecompressionQueueEvent = g_customDecompressionQueue->GetEvent();
    g_customDecompressionRequestsAvailableWait =
        CreateThreadpoolWait(OnCustomDecompressionRequestsAvailable, nullptr, nullptr);
    SetThreadpoolWait(g_customDecompressionRequestsAvailableWait, g_customDecompressionQueueEvent, nullptr);

    g_decompressionWork = CreateThreadpoolWork(DecompressionWork, nullptr, nullptr);
#endif
}

void PhxEngine::Core::DStorageLoader::Finalize()
{
}
