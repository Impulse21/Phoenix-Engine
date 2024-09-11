#pragma once

#include <Core/phxPrimitives.h>
#include <memory>
#include <vector>

#include <Core/phxMath.h>
struct cgltf_primitive;

namespace phx::MeshConverter
{
    struct Primitive
    {
        Sphere BoundsLS;    // local space bounds
        Sphere BoundsOS;    // object space bounds
        AABB BBoxLS;        // local space AABB
        AABB BBoxOS;        // object space AABB
        std::shared_ptr<uint8_t[]> VertexBuffer;
        uint32_t VertexBufferSize;
        std::shared_ptr<uint8_t[]> IndexBuffer;
        uint32_t IndexBufferSize;
        uint32_t NumVertices;
        uint32_t NumIndices;
        uint32_t PrimCount;
        union
        {
            uint32_t Hash;
            struct {
                uint32_t PsoFlags : 16;
                uint32_t Index32 : 1;
                uint32_t MaterialIdx : 15;
            };
        };
    };

    void OptimizeMesh(
        Primitive& outPrim,
        cgltf_primitive const& inPrim,
        DirectX::XMMATRIX const& localToObject);
}