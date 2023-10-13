#include <PhxEngine/Renderer/AsyncGpuUploader.h>
#include <PhxEngine/Core/Log.h>

using namespace PhxEngine::Core;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;

void PhxEngine::Renderer::AsyncGpuUploader::Initialize()
{
}

void PhxEngine::Renderer::AsyncGpuUploader::OnTick()
{
	{
		std::scoped_lock _(this->m_fileLoadQueueMutex);
		while (!this->m_fileLoadRequests.empty())
		{
			FileLoadRequest& request = this->m_fileLoadRequests.front();

			// TODO: Do work
			PHX_LOG_CORE_INFO("Processing File Load Request '%s'", request.FilePath.c_str());

			this->m_fileLoadRequests.pop_front();
		}
	}
	{
		std::scoped_lock _(this->m_gpuUploadRequestMutex);
		while (!this->m_gpuUploadRequests.empty())
		{
			GpuUploadRequest& request = this->m_gpuUploadRequests.front();

			// TODO: Do work
			PHX_LOG_CORE_INFO("Processing GPU Upload Request");

			this->m_gpuUploadRequests.pop_front();
		}
	}
}

void PhxEngine::Renderer::AsyncGpuUploader::Finalize()
{
}

void PhxEngine::Renderer::AsyncGpuUploader::EnqueueTextureUpload(std::filesystem::path FilePath, RHI::TextureHandle handle)
{
	std::scoped_lock _(this->m_fileLoadQueueMutex);
	FileLoadRequest& request = this->m_fileLoadRequests.emplace_back();
	request.FilePath = FilePath;
	request.Texture = handle;
}

void PhxEngine::Renderer::AsyncGpuUploader::EnqueueBufferUpload(void* data, RHI::BufferHandle handle)
{
	std::scoped_lock _(this->m_gpuUploadRequestMutex);
	GpuUploadRequest& request = this->m_gpuUploadRequests.emplace_back();
	request.Data = data;
	request.GpuBuffer = handle;
}

void PhxEngine::Renderer::AsyncGpuUploader::EnqueueBufferCopy(RHI::BufferHandle src, RHI::BufferHandle dest)
{
	std::scoped_lock _(this->m_gpuUploadRequestMutex);
	GpuUploadRequest& request = this->m_gpuUploadRequests.emplace_back();
	request.CpuBuffer = src;
	request.GpuBuffer = dest;
}
