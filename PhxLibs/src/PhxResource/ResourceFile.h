#pragma once

#include "ResourceFileFormat.h"
#include "IAssetStreamer.h"

namespace phx
{
	struct ResourceFile
	{
		std::shared_ptr<IAssetStreamer> const& AssetStreamer;
		StreamFileHandle FileHandle;
		ResourceFileFormat::Header Header = {};

		std::vector<StreamCallback> BespokeChunkCallbacks;

		void BeginResourceLoad()
		{

		}
	};
}

