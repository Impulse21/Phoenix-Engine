#pragma once

#include "IAssetStreamer.h"

namespace phx
{
	class ChunkFile
	{
	public:
		ChunkFile(std::shared_ptr<IAssetStreamer> assetStreamer, StreamFileHandle handle);
		ChunkFile(std::shared_ptr<IAssetStreamer> assetStreamer, std::filesystem::path const& path);

	private:
		const bool m_ownsFileHandle;
		std::filesystem::path m_filePath;
		std::string m_cachedFilename;
		std::shared_ptr<IAssetStreamer> m_assetStreamer;
		StreamFileHandle m_fileHandle;
		std::atomic_uint8_t m_status;
	};
}

