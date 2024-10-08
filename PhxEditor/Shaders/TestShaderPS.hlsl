
struct VSOutput
{
    float4 color : COLOR; // Output color to the pixel shader
};

// Pixel Shader - Simply outputs the color from the vertex shader
float4 main(VSOutput input) : SV_Target
{
    return input.color; // Output the interpolated color
}
