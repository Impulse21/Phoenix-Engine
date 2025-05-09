#pragma once

#include <PhxData/Reflection.h>
#include <PhxData/DataContainers.h>

#include <DirectXMath.h>

namespace phx
{
	struct Level
	{
		data::String Name;
		LevelObject RootObject;

		PHX_REFLECT(Level)
			PHX_REFLECT_FIELD(Name)
			PHX_REFLECT_FIELD(RootObject)
		PHX_REFLECT_END()
	};

	struct LevelObject
	{
		data::String Name;
		DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 Translation = { 0.0f, 0.0f, 0.0f };

		data::FlexArray<LevelObject> Children;

		PHX_REFLECT(LevelObject)
			PHX_REFLECT_FIELD(Name)
			PHX_REFLECT_FIELD(Scale)
			PHX_REFLECT_FIELD(Rotation)
			PHX_REFLECT_FIELD(Translation)
			PHX_REFLECT_FIELD(Children)
		PHX_REFLECT_END()
	};
}