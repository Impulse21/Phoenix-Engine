#pragma pack_matrix(row_major)

#include "Globals_NEW.hlsli"
#include "../Include/PhxEngine/Shaders/ShaderInterop.h"

PUSH_CONSTANT(push, CullPushConstants);


// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
bool ProjectSphere(float3 centre, float radius, float znear, float P00, float P11, out float4 aabb)
{
    if (-centre.z - radius < znear)
    {
        return false;
    }

#if false
    float2 cx = float2(centre.x, -centre.z);
    float2 vx = float2(sqrt(dot(cx, cx) - radius * radius), radius);
    float2 minx = float2x2(vx.x, vx.y, -vx.y, vx.x) * cx;
    float2 maxx = float2x2(vx.x, -vx.y, vx.y, vx.x) * cx;

    float2 cy = -centre.yz;
    float2 vy = float2(sqrt(dot(cy, cy) - r * r), r);
    float2 miny = float2x2(vy.x, vy.y, -vy.y, vy.x) * cy;
    float2 maxy = float2x2(vy.x, -vy.y, vy.y, vy.x) * cy;

    aabb = float4(minx.x / minx.y * P00, miny.x / miny.y * P11, maxx.x / maxx.y * P00, maxy.x / maxy.y * P11);
    aabb = aabb.xwzy * float4(0.5f, -0.5f, 0.5f, -0.5f) + 0.5f; // clip space -> uv space
#endif
    return true;
}

[RootSignature(PHX_ENGINE_DEFAULT_ROOTSIGNATURE)]
[numthreads(THREADS_PER_WAVE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint objectInstanceIdx = DTid.x;
    
    uint totalCount = GetScene().InstanceCount;
    if (push.IsLatePass)
    {
        ByteAddressBuffer counterBuffer = ResourceDescriptorHeap[push.CulledDataCounterSrcIdx];
        totalCount = counterBuffer.Load(0);
    }
    
    if (objectInstanceIdx < totalCount)
    {
        if (push.IsLatePass)
        {
            StructuredBuffer<MeshDrawCommand> culledDrawCommand = ResourceDescriptorHeap[push.CulledDataCounterSrcIdx];
            objectInstanceIdx = culledDrawCommand[objectInstanceIdx].DrawId;
        }
        
        ObjectInstance objectInstance = LoadObjectInstnace(objectInstanceIdx);
        Geometry geometryData = LoadGeometry(objectInstance.GeometryIndex);
    
        float4 geometryBoundingSphere = LoadGeometryBounds(objectInstance.GeometryIndex);
        
        // Transform into view projection space
        float4 boundCentreWS = mul(float4(geometryBoundingSphere.xyz, 1.0f), objectInstance.WorldMatrix);
        float4 boundCentreVS = mul(boundCentreWS, GetCamera().ViewProjection);
        // float radius = geometryBoundingSphere.w * 1.1;
        float radius = geometryBoundingSphere.w;
        
        bool isVisibleFrustum = true;
        for (uint i = 0; i < 6; i++)
        {
            float d = dot(GetCamera().PlanesWS[i], boundCentreWS);
            isVisibleFrustum = isVisibleFrustum && (d > -radius);
        }
        
        bool isVisibileOcclusion = true;
        if (isVisibleFrustum)
        {
#if 0
            float4 aabb;
            if (ProjectSphere(boundCentreVS, radius, GetCamera().GetZNear(), GetCamera().Proj[0][0], GetCamera().Proj[1][1], aabb))
            {
                // TODO: I AM HERE.
                Texture2D depthPyramidTex = ResourceDescriptorHeap[GetFrame().DepthPyramidIndex];
                uint depthPyramidWidth = 0;
                uint depthPyramidHeight = 0;
                depthPyramidTex.GetDimensions(depthPyramidWidth, depthPyramidHeight);
                
                float width = (aabb.z - aabb.x) * depthPyramidWidth;
                float height = (aabb.w - aabb.y) * depthPyramidHeight;

                float level = floor(log2(max(width, height)));
                
                float2 uv = (aabb.xy + aabb.zw) * 0.5;
                uv.y = 1 - uv.y;
                
                float depth = depthPyramidTex.SampleLevel(SamplerDefault, uv, level).r;
               // Sample also 4 corners
                depth = max(depth, textureLod(global_textures[nonuniformEXT(depth_pyramid_texture_index)], vec2(aabb.x, 1.0f - aabb.y), level).r);
                depth = max(depth, textureLod(global_textures[nonuniformEXT(depth_pyramid_texture_index)], vec2(aabb.z, 1.0f - aabb.w), level).r);
                depth = max(depth, textureLod(global_textures[nonuniformEXT(depth_pyramid_texture_index)], vec2(aabb.x, 1.0f - aabb.w), level).r);
                depth = max(depth, textureLod(global_textures[nonuniformEXT(depth_pyramid_texture_index)], vec2(aabb.z, 1.0f - aabb.y), level).r);
				// Sampler is set up to do max reduction, so this computes the minimum depth of a 2x2 texel quad
                float depthSphere = cullData.znear / (boundCentreVS.z - radius);
                
                //if the depth of the sphere is in front of the depth pyramid value, then the object is visible
                isVisibileOcclusion = depthSphere <= depth;
            }
#endif

        }
        
        if (isVisibleFrustum && isVisibileOcclusion)
        {
            MeshDrawCommand command;
            command.DrawId = objectInstanceIdx;
            command.Indirect.IndexCount = geometryData.NumIndices;
            command.Indirect.InstanceCount = 1;
            command.Indirect.StartIndex = geometryData.IndexOffset;
            command.Indirect.VertexOffset = 0;
            command.Indirect.StartInstance = 0;
            command.IndirectMS.GroupCountX = ROUNDUP(geometryData.MeshletCount, AS_GROUP_SIZE);
            command.IndirectMS.GroupCountY = 1;
            command.IndirectMS.GroupCountZ = 1;
    
            AppendStructuredBuffer<MeshDrawCommand> drawMeshIndirectBuffer = ResourceDescriptorHeap[push.DrawBufferIdx];
            drawMeshIndirectBuffer.Append(command);
        }
        else if (push.IsLatePass == false)
        {
            MeshDrawCommand command;
            command.DrawId = objectInstanceIdx;
            command.IndirectMS.GroupCountX = ROUNDUP(geometryData.MeshletCount, AS_GROUP_SIZE);
            command.IndirectMS.GroupCountY = 1;
            command.IndirectMS.GroupCountZ = 1;
            // AppendStructuredBuffer<MeshDrawCommand> culledIndirectBuffer = ResourceDescriptorHeap[GetScene().CulledInstancesBufferUavIdx];
            // culledIndirectBuffer.Append(command);
        }
    }
}