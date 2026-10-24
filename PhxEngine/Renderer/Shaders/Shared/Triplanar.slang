module TriplanarUtils;

public float3 CalculateTriplanarBlendWeights(float3 normal)
{
    float3 weights = abs(normal);
    weights        = (weights - 0.2) * 7.0; // Nvidia GPU-Gems 3, Chapter 1
    weights /= (weights.x + weights.y + weights.z);

    weights = max(weights, 0.0);

    return weights;
}

float4 TriplanarSample(Texture2D<float4> texture, SamplerState sampler, float3 world_pos, float3 weights)
{
    float2 yzPlane = float2(world_pos.y, world_pos.z);
    float2 xzPlane = float2(world_pos.x, world_pos.z);
    float2 zyPlane = float2(world_pos.x, world_pos.y);

    float4
}