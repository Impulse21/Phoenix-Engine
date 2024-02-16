#pragma once

#include "PipelineTypes.h"

#include <PhxEngine/Core/VirtualFileSystem.h>

namespace PhxEngine::Pipeline
{
	class TextureConvertPipeline
	{
	public:
		TextureConvertPipeline(Core::IFileSystem* FileSystem);

		void Convert(Texture& texture);

	private:
		Core::IFileSystem* m_fileSystem;
	};
}

