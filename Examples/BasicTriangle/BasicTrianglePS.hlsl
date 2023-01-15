void main(
	in float3 inColour : COLOR,
	out float4 outColour : SV_Target0
)
{
    outColour = float4(inColour, 1);
}
