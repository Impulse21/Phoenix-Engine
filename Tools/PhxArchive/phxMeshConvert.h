#pragma once

#include <Core/phxPrimitives.h>
#include <memory>
#include <vector>

#include <Core/phxMath.h>
struct cgltf_primitive;

namespace phx::MeshConverter
{
    using ByteArray = std::shared_ptr<std::vector<uint8_t>>;

    struct Primitive
    {
        Sphere BoundsLS;    // local space bounds
        Sphere BoundsOS;    // object space bounds
        AABB BBoxLS;        // local space AABB
        AABB BBoxOS;        // object space AABB
        ByteArray VB;
        ByteArray IB;
        ByteArray DepthVB;
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
        uint16_t VertexStride;
    };
    void OptimizeMesh(
        Primitive& outPrim,
        cgltf_primitive const& inPrim,
        DirectX::XMMATRIX const& localToObject);
}