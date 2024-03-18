#pragma once

#include "PipelineTypes.h"

#include <PhxEngine/Core/VirtualFileSystem.h>

namespace PhxEngine::Pipeline
{
	class TextureConvertPipeline
	{
	public:
		TextureConvertPipeline(IFileSystem* FileSystem);

		void Convert(Texture& texture);

	private:
		IFileSystem* m_fileSystem;
	};
}

