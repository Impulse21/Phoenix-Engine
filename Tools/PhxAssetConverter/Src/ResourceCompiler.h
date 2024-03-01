#pragma once

#include "PipelineTypes.h"
namespace PhxEngine::Pipeline
{
	class ResourceCompiler
	{
	protected:
		ResourceCompiler()
		{}

	protected:
	};

	class MeshResourceCompiler : public ResourceCompiler
	{
	public:
		static void Compile(std::vector<uint8_t>& bufferMemory, Mesh const& mesh)
		{
			MeshResourceCompiler compiler(mesh);
			compiler.Compile();
		}

	private:
		MeshResourceCompiler(Mesh const& mesh);
		void Compile();

	private:
		const Mesh& m_mesh;
	};

	class TextureResourceCompiler : public ResourceCompiler
	{
	public:
		static void Compile(Texture const& texture)
		{
			TextureResourceCompiler compiler(texture);
			compiler.Compile();
		}

	private:
		TextureResourceCompiler(Texture const& texture);
		void Compile();

	private:
		const Texture& m_texture;
	};

	class MaterialResourceCompiler : public ResourceCompiler
	{
	public:
		static void Compile(Material const& mtl)
		{
			MaterialResourceCompiler compiler(mtl);
			compiler.Compile();
		}

	private:
		MaterialResourceCompiler(Material const& mtl);
		void Compile();

	private:
		const Material& m_material;
	};
}

