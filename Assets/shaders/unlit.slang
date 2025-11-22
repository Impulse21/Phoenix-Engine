// --- Data Structures ---

struct Vertex
{
    float3 position;
    float2 uv;
    float4 color;
};

// We use pointers for data buffers (VK_KHR_buffer_device_address)
struct MaterialData
{
    float4x4 modelViewProjection;
    float4 tintColor;
    int textureIndex; // This is the offset/index into your Descriptor Buffer
    float blendFactor;
};

// --- Pointers & Push Constants ---

struct PushConstants
{
    Vertex* vertices;      // Address of Vertex Buffer
    MaterialData* material; // Address of Material Buffer
};

[[vk::push_constant]] PushConstants pc;

// --- Bindless Descriptor Heap (VK_EXT_descriptor_buffer) ---

// Even with the extension, Shaders view the descriptor buffer as an array.
// The API binds the start of the Descriptor Buffer to Set 0.
// 'textureIndex' in MaterialData tells us which descriptor to read from that buffer.
[[vk::binding(0, 0)]] 
Texture2D<float4> descriptorHeap[]; 

// Static sampler (or fetched from a sampler heap)
[[vk::binding(1, 0)]] 
SamplerState mainSampler;

// --- Generics Helper ---

// Helper to safely sample from the heap using an index
T SampleHeap<T>(Texture2D<T> heap[], SamplerState s, int index, float2 uv, T fallback)
{
    // Index -1 implies "no texture"
    if (index < 0) return fallback;

    // NonUniformResourceIndex is crucial! 
    // It tells the compiler the index varies per-pixel (dynamically),
    // which prevents it from assuming a constant texture for the draw call.
    return heap[NonUniformResourceIndex(index)].Sample(s, uv);
}

// --- Shader I/O ---

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR;
};

// --- Vertex Shader ---
[shader("vertex")]
VSOutput vertexMain(uint vertexID : SV_VertexID)
{
    VSOutput output;

    // 1. Vertex Pulling via Pointer
    // Loads directly from the GPU address specified in Push Constants
    Vertex v = pc.vertices[vertexID];

    // 2. Material Data via Pointer
    float4x4 mvp = pc.material->modelViewProjection;

    output.position = mul(mvp, float4(v.position, 1.0));
    output.uv = v.uv;
    output.color = v.color;

    return output;
}

// --- Fragment (Pixel) Shader ---
[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target
{
    // 1. Get Material Data via Pointer
    MaterialData* mat = pc.material;
    
    // 2. Sample from Descriptor Buffer
    // We use the integer index from the material data to find the
    // specific descriptor in the bound heap.
    float4 texColor = SampleHeap(
        descriptorHeap, 
        mainSampler, 
        mat->textureIndex, 
        input.uv, 
        input.color // Fallback to vertex color if index is -1
    );

    // 3. Blend and Tint
    float4 finalColor = lerp(input.color, texColor, mat->blendFactor);
    return finalColor * mat->tintColor;
}