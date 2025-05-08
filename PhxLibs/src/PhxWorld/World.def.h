#pragma once

#include <PhxCore/UUID.h>
#include <PhxData/DataContainers.h>
#include <DirectXMath.h>

namespace phx
{
    struct Component
    {
        UUID ID;
    };

    struct MeshComponent
    {
        data::String Mesh;
    };

    struct WorldObject
    {
        UUID ID;
        data::String Name = "";

        DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT4 Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 Translation = { 0.0f, 0.0f, 0.0f };
    };
}