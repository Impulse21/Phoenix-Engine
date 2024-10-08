#include "MeshCompiler.h"

void PhxEngine::Pipeline::MeshCompiler::CompileGeometryData()
{
	size_t indexSize = this->m_mesh.Indices.size() * sizeof(this->m_mesh.Indices.front());

	size_t vertexBufferSize = 0;
	for (auto& stream : this->m_mesh.VertexStreams)
	{
		vertexBufferSize += stream.GetNumElements() * stream.GetElementStride();
	}

	const size_t totalGeometryDataSize = indexSize + vertexBufferSize;
	this->m_compiledMesh.Geometry.resize(totalGeometryDataSize);

	for (auto& meshPart : this->m_mesh.MeshParts)
	{
	}
}
