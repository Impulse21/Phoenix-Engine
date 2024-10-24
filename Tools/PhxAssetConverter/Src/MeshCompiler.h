#pragma once

#include "PipelineTypes.h"

namespace PhxEngine::Pipeline
{
	class MeshCompiler
	{
	public:
		static CompiledMesh Compile(Mesh const& mesh)
		{
			MeshCompiler compiler(mesh);
			return compiler.Compile();
		}

	private:
		MeshCompiler(Mesh const& mesh)
			: m_mesh(mesh)
			, m_compiledMesh({})
		{
		}

		CompiledMesh Compile()
		{
			this->CompileGeometryData();
			return this->m_compiledMesh;
		}


		void CompileGeometryData();

	private:
		const Mesh & m_mesh;
		CompiledMesh m_compiledMesh;
	};

}