static const float2 gPositions[] =
{
    float2(-0.5, -0.5),
	float2(0, 0.5),
	float2(0.5, -0.5)
};

static const float3 gColors[] =
{
    float3(1, 0, 0),
	float3(0, 1, 0),
	float3(0, 0, 1)
};

struct Payload
{
    int Dummy;
};

groupshared Payload s_payload;


[numthreads(1, 1, 1)]
void main(uint globalIdx : SV_DispatchThreadID)
{
    s_payload.Dummy = 0;
    DispatchMesh(1, 1, 1, s_payload);
}
