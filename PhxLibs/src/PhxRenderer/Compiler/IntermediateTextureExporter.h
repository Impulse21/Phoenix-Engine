#pragma once


namespace phx::renderer::compiler
{
	struct IntermediateTexture;

	namespace IntermediateTextureExporter
	{
		bool Export(IntermediateTexture const& texture, std::ostream& out);
		bool ExportBC7ToDDS(IntermediateTexture const& texture, std::ostream& out);
	}
}
