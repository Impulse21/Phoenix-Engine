#include "phxMeshConvert.h"

#include <Core/phxLog.h>
#include <cgltf/cgltf.h>
#include <mesh-optimizer/meshoptimizer.h>

using namespace phx;
using namespace phx::MeshConverter;

void phx::MeshConverter::OptimizeMesh(
	Primitive& outPrim,
	cgltf_primitive const& inPrim,
	DirectX::XMMATRIX const& localToObject)
{
	// TODO: I AM HERE

    if (inPrim.type != cgltf_primitive_type_triangles ||
        inPrim.attributes_count == 0)
    {
        PHX_INFO("Found unsupported primitive topology");
        return;
    }

#if false

	size_t indexCount = inPrim.ac;
	size_t unindexed_vertex_count = face_count * 3;
	std::vector<unsigned int> remap(index_count); // allocate temporary memory for the remap table
	size_t vertex_count = meshopt_generateVertexRemap(&remap[0], NULL, index_count, &unindexed_vertices[0], unindexed_vertex_count, sizeof(Vertex));

#endif 

	if (inPrim.indices == nullptr)
	{

	}
}
