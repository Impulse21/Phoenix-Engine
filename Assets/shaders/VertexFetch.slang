
module VertexFetch;

public static const uint kInvalidOffset = 0xFFFFFFFF;
public static const uint STREAM_POSITION  = 0;
public static const uint STREAM_TANGENT   = 1;
public static const uint STREAM_NORMAL    = 2;
public static const uint STREAM_TEXCOORD0 = 3;
public static const uint STREAM_TEXCOORD1 = 4;
public static const uint STREAM_COLOUR0   = 5;
public static const uint STREAM_JOINTS0   = 6;
public static const uint STREAM_WEIGHTS0  = 7;
public static const uint STREAM_COUNT     = 8;

public struct VertexStreamDesc
{
    private uint Stride4_Offset28;

    // Unpack 28-bit Offset
    public uint GetOffset()
    {
        return Stride4_Offset28 & 0x0FFFFFFF;
    }

    // Unpack 4-bit Stride
    public uint GetStride()
    {
        return (Stride4_Offset28 >> 28) & 0xF;
    }
};

// Matches C++ struct VertexStreamsHeader
public struct VertexStreamsHeader
{
    public VertexStreamDesc desc[STREAM_COUNT];
};

public struct VertexInputStream
{
    private void* m_base_ptr;
    private VertexStreamsHeader* m_header;

    public __init(void *ptr)
    {
        m_base_ptr = ptr;
        m_header = (VertexStreamsHeader *)ptr;
    }

    private T LoadAttributeOr<T>(VertexStreamDesc* desc,  uint index, T default)
    {
        const uint32_t offset = desc->GetOffset();

        if (offset == kInvalidOffset)
            return default;

        uint8_t *byte_base = (uint8_t *)m_base_ptr;
        const uint32_t stride = desc->GetStride();
        uint8_t *address = byte_base + offset + (index * stride);

        return *((T *)address);
    }

    // --- Public API ---
    public float3 GetPosition(uint vertexID)
    {
        return LoadAttributeOr<float3>(&m_header.desc[STREAM_POSITION], vertexID, float3(0, 0, 0));
    }

    public float3 GetNormal(uint vertexID)
    {
        return LoadAttributeOr<float3>(&m_header.desc[STREAM_NORMAL], vertexID, float3(0, 0, 0));
    }

    public float4 GetColor(uint vertexID)
    {
        VertexStreamDesc *desc = &m_header.desc[STREAM_COLOUR0];
        if (desc->GetStride() == 4)
        {
            uint packedColor = LoadAttributeOr<uint>(desc, vertexID, 0);

            // Manual unpack (Unorm)
            return float4(
                (packedColor & 0xFF) / 255.0,
                ((packedColor >> 8) & 0xFF) / 255.0,
                ((packedColor >> 16) & 0xFF) / 255.0,
                ((packedColor >> 24) & 0xFF) / 255.0);
        }
        else
        {
            return LoadAttributeOr<float4>(desc, vertexID, float4(0, 0, 0, 0));
        }
    }

    public float2 GetUV(uint vertexID)
    {
        return LoadAttributeOr<float2>(&m_header.desc[STREAM_TEXCOORD0], vertexID, float2(0, 0));
    }
}