#pragma once

#include "Core/phxMath.h"
#include "phxBaseInclude.h"

namespace phx
{
	class IResource
	{
	public:
		virtual ~IResource() = default;
	};


    //
    // To request a PSO index, provide flags that describe the kind of PSO
    // you need.  If one has not yet been created, it will be created.
    //
    namespace PSOFlags
    {
        enum : uint16_t
        {
            kHasPosition = BIT(1),
            kHasNormal = BIT(2),  // Required
            kHasTangent = BIT(3),
            kHasUV0 = BIT(4),  // Required (for now)
            kHasUV1 = BIT(5),
            kAlphaBlend = BIT(6),
            kAlphaTest = BIT(7),
            kTwoSided = BIT(8),
            kHasSkin = BIT(9),  // Implies having indices and weights
        };
    }
    struct Mesh
    {
        float    Bounds[4];     // A bounding sphere
        uint32_t VbOffset;      // BufferLocation - Buffer.GpuVirtualAddress
        uint32_t VbSize;        // SizeInBytes
        uint32_t IbOffset;      // BufferLocation - Buffer.GpuVirtualAddress
        uint32_t IbSize;        // SizeInBytes
        uint8_t  VbStride;      // StrideInBytes
        uint8_t  IbFormat;      // DXGI_FORMAT
        uint16_t MeshCBV;       // Index of mesh constant buffer
        uint16_t MaterialCBV;   // Index of material constant buffer
        uint16_t SrvTable;      // Offset into SRV descriptor heap for textures
        uint16_t SamplerTable;  // Offset into sampler descriptor heap for samplers
        uint16_t PsoFlags;      // Flags needed to request a PSO
        uint16_t Pso;           // Index of pipeline state object
        uint16_t NumJoints;     // Number of skeleton joints when skinning
        uint16_t StartJoint;    // Flat offset to first joint index
        uint16_t NumDraws;      // Number of draw groups

        struct DrawInfo
        {
            uint32_t IndexCount;  // Number of indices
            uint32_t StartIndex;  // Offset to first index in index buffer 
            uint32_t BaseVertex;  // Offset to first vertex in vertex buffer
        };
        DrawInfo Draw[1];           // Actually 1 or more draws
    };

    struct GraphNode
    {
        DirectX::XMFLOAT4X4 XForm;;
        DirectX::XMFLOAT4 Rotation; //quaterion
        DirectX::XMFLOAT3 Scale;

        uint32_t MatrixIdx : 28;
        uint32_t HasSibling : 1;
        uint32_t HasChildren : 1;
        uint32_t StaleMatrix : 1;
        uint32_t SkeletonRoot : 1;
    };
    static_assert(sizeof(GraphNode) == 96);
}

