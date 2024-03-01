#pragma once

#include "PipelineTypes.h"

#include <PhxEngine/Core/VirtualFileSystem.h>
#include "TextureConvertPipeline.h"

namespace PhxEngine::Pipeline
{
	class TextureConvertPipeline
	{
	public:
		TextureConvertPipeline(Core::IFileSystem* FileSystem);

		void Convert(Texture& texture, TexConversionFlags additionalFlags);

	private:
		Core::IFileSystem* m_fileSystem;
	};
}

