

// Define the positions of the triangle vertices based on the vertexID
float3 trianglePos[3] =
{
    float3(0.0f, 0.5f, 0.0f), // Top vertex
    float3(0.5f, -0.5f, 0.0f), // Bottom-right vertex
    float3(-0.5f, -0.5f, 0.0f) // Bottom-left vertex
};

float3 triangleColours[3] =
{
    float3(1.0f, 0.0, 0.0f), // Top vertex
    float3(0.0f, 1.0f, 0.0f), // Bottom-right vertex
    float3(0.0f, 0.50, 1.0f) // Bottom-left vertex
};

struct VSOutput
{
    float4 color : COLOR; // Output color to the pixel shader
    float4 position : SV_POSITION; // Output position to the rasterizer
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;
    
    // Assign the position of the vertex
    output.position = float4(trianglePos[vertexID], 1.0f);

    // Assign a color to each vertex (you can customize this)
    output.color = float4(triangleColours[vertexID], 1.0f); // Red color for now

    return output;
}