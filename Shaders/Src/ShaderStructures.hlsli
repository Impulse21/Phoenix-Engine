#ifndef __PHX_SHADER_STRUCTURES_HLSLI__
#define __PHX_SHADER_STRUCTURES_HLSLI__

#define PUSHCONSTANT(name, type) ConstantBuffer<type> name : register(b999)
struct DrawPushConstant
{
    
};

struct Scene
{
    
};

struct Frame
{
    Scene sceneData;
};

struct Camera
{
    
};


#endif // __PHX_SHADER_STRUCTURES_HLSLI__