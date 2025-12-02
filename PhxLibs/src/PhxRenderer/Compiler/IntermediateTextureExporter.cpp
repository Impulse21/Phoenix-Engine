#include "PhxRenderer/PhxRenderer_pch.h"
#include "IntermediateTextureExporter.h"

#include <PhxCore/BinaryBuilder.h>
#include <PhxResource/ResourceFileFormat.h>

#include <PhxRenderer/TextureResourceHandler.h>
#include <PhxRenderer/TextureResource.h>

#include "IntermediateTexture.h"

using namespace phx;
using namespace phx::renderer;
using namespace phx::renderer::compiler;

bool IntermediateTextureExporter::Export(IntermediateTexture const& texture, std::ostream& out)
{
	BinaryBuilder file_builder;


	const OffsetHandle header_offset = file_builder.Reserve<ResourceFileFormat::Header>();
	const OffsetHandle metadata_header_offset = file_builder.Reserve<ResourceFileFormat::MetadataHeader>();
    return false;
}
