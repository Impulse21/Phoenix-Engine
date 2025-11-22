
module VertexFetch;

static const uint STREAM_POSITION  = 0;
static const uint STREAM_TANGENT   = 1;
static const uint STREAM_NORMAL    = 2;
static const uint STREAM_TEXCOORD0 = 3;
static const uint STREAM_TEXCOORD1 = 4;
static const uint STREAM_COLOUR0   = 5;
static const uint STREAM_JOINTS0   = 6;
static const uint STREAM_WEIGHTS0  = 7;
static const uint STREAM_COUNT     = 8;

struct VertexStreamDesc
{
    uint Stride4_Offset28;

    // Unpack 28-bit Offset
    uint GetOffset()
    {
        return Stride4_Offset28 & 0x0FFFFFFF;
    }

    // Unpack 4-bit Stride
    uint GetStride()
    {
        return (Stride4_Offset28 >> 28) & 0xF;
    }
};

// Matches C++ struct VertexStreamsHeader
struct VertexStreamsHeader
{
    VertexStreamDesc desc[STREAM_COUNT];
};