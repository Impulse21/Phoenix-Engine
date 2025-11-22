import VertexFetch;

// --- Pointers & Push Constants ---

struct VSOutput
{
    float4 position : SV_Position;
    float4 colour    : COLOR;
};

// --- Vertex Shader ---
[shader("vertex")]
VSOutput VertexMain(
    uint vertexID: SV_VertexID,
    uniform float4x4 modelViewProjection,
    uniform VertexStreamsHeader *bufferPtr)
{
    VSOutput output;

    uint8_t *rawBuffer = (uint8_t *)bufferPtr;

    VertexStreamDesc posDesc = bufferPtr->desc[STREAM_POSITION];
    uint posOffset = posDesc.GetOffset();
    uint posStride = posDesc.GetStride();
    float3 *posPtr = (float3 *)(rawBuffer + posOffset + (vertexID * posStride));
    float3 position = *posPtr;

    VertexStreamDesc colDesc = bufferPtr->desc[STREAM_COLOUR0];
    uint colOffset = colDesc.GetOffset();
    uint colStride = colDesc.GetStride(); // Likely 4 bytes if packed RGBA
    float4 color;
    if (colStride == 4)
    {
        // Load as uint, unpack to float4
        uint *colPtr = (uint *)(rawBuffer + colOffset + (vertexID * colStride));
        uint packedColor = *colPtr;

        // Manual unpack (Unorm)
        color = float4(
            (packedColor & 0xFF) / 255.0,
            ((packedColor >> 8) & 0xFF) / 255.0,
            ((packedColor >> 16) & 0xFF) / 255.0,
            ((packedColor >> 24) & 0xFF) / 255.0
        );
    }
    else
    {
        // Fallback or assume float4 (requires stride > 15 which is impossible in 4 bits!)
        // Assuming white if invalid setup
        // color = float4(1.0, 1.0, 1.0, 1.0);
        color = float4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    float4x4 mvp = modelViewProjection;
    output.position = mul(modelViewProjection, float4(position, 1.0));
    output.colour = color;

    return output;
}

// --- Fragment (Pixel) Shader ---
[shader("fragment")]
float4 FragmentMain(VSOutput input) : SV_Target
{
    return input.colour;
}