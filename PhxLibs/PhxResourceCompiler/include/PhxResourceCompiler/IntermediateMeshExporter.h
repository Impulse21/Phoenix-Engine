#pragma once

#include <string>

namespace phx::resource::compiler
{
	struct IntermediateMesh;

	class IntermediateMeshExporter
	{
	public:
		static bool Export(IntermediateMesh const& intermediate_mesh, std::ostream& out)
		{
			IntermediateMeshExporter exporter(intermediate_mesh, out);
			return exporter();
		}

	protected:
		IntermediateMeshExporter(IntermediateMesh const& intermediate_mesh, std::ostream& out);
		bool operator()();

	private:
		const IntermediateMesh& m_intermediate_mesh;
		std::ostream& m_out;
	};
}

