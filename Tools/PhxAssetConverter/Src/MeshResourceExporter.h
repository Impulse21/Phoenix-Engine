#pragma once

#include <ostream>
#include <PhxEngine/Resource/MeshFileFormat.h>
#include "PipelineTypes.h"

namespace PhxEngine
{
	class BinaryBuilder;
}

namespace PhxEngine::Pipeline
{
	class MeshResourceExporter
	{
    public:
        static void Export(
            std::ostream& out,
            Compression compression,
			Mesh const& mesh)
        {
			MeshResourceExporter exporter(out, compression, mesh);
            exporter.Export();
        }

	private:
		MeshResourceExporter(std::ostream& out, Compression compression, Mesh const& mesh);

		void Export();

	private:
		Region<MeshFileFormat::MeshMetadataHeader> CreateMeshMetadataRegion(BinaryBuilder& regionBuilder);
		GpuRegion CreateGpuRegion(BinaryBuilder& regionBuilder);

	private:
		const Mesh& m_mesh;
		std::ostream& m_out;
		Compression m_compression;
	
	};
}

