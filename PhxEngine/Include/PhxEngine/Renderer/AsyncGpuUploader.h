#pragma once

#include <deque>
#include <filesystem>

#include <PhxEngine/RHI/PhxRHI.h>

namespace PhxEngine::Renderer
{
	class AsyncGpuUploader
	{
	public:
		AsyncGpuUploader() = default;
		void Initialize();

		void OnTick();

		void Finalize();

		void EnqueueTextureUpload(std::filesystem::path FilePath, RHI::TextureHandle handle);
		void EnqueueBufferUpload(void* data, RHI::BufferHandle handle);
		void EnqueueBufferCopy(RHI::BufferHandle src, RHI::BufferHandle dest);

	private:
		struct FileLoadRequest
		{
			std::filesystem::path FilePath;
			PhxEngine::RHI::TextureHandle Texture;
			PhxEngine::RHI::BufferHandle Buffer;
		};

		struct GpuUploadRequest
		{
			void* Data = nullptr;
			uint32_t* Completed = nullptr;
			PhxEngine::RHI::TextureHandle Texture;
			PhxEngine::RHI::BufferHandle CpuBuffer;
			PhxEngine::RHI::BufferHandle GpuBuffer;
		};

		std::mutex m_fileLoadQueueMutex;
		std::deque<FileLoadRequest> m_fileLoadRequests;

		std::mutex m_gpuUploadRequestMutex;
		std::deque<GpuUploadRequest> m_gpuUploadRequests;
	};
}

