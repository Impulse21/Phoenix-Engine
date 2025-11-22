
module VertexFetch;

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