#pragma once

#include "PipelineTypes.h"
#include <ostream>

namespace PhxEngine::Pipeline
{
	class ICompressor;
	class ResourceExporter
	{
	protected:
		ResourceExporter(std::ostream& out, ICompressor* compressor)
			: m_out(out)
			, m_compressor(compressor)
		{}

	protected:
		std::ostream& m_out;
		ICompressor* m_compressor;
	};

	class MeshResourceExporter : public ResourceExporter
	{
	public:
		static void Export(std::ostream& out, ICompressor* compressor, Mesh const& mesh)
		{
			MeshResourceExporter exporter(out, compressor, mesh);
			exporter.Export();
		}

	private:
		MeshResourceExporter(std::ostream& out, ICompressor* compressor, Mesh const& mesh);
		void Export();

	private:
		const Mesh& m_mesh;
	};

	class TextureResourceExporter : public ResourceExporter
	{
	public:
		static void Export(std::ostream& out, ICompressor* compressor, Texture const& texture)
		{
			TextureResourceExporter exporter(out, compressor, texture);
			exporter.Export();
		}

	private:
		TextureResourceExporter(std::ostream& out, ICompressor* compressor, Texture const& texture);
		void Export();

	private:
		const Texture& m_texture;
	};

	class MaterialResourceExporter : public ResourceExporter
	{
	public:
		static void Export(std::ostream& out, ICompressor* compressor, Material const& mtl)
		{
			MaterialResourceExporter exporter(out, compressor, mtl);
			exporter.Export();
		}

	private:
		MaterialResourceExporter(std::ostream& out, ICompressor* compressor, Material const& mtl);
		void Export();

	private:
		const Material& m_material;
	};
}

