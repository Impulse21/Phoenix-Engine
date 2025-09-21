#pragma once

#include <PhxReflection/Reflection.h>
#include <PhxData/DataContainers.h>
#include <PhxData/DataPtr.h>

#include <hlsl++.h>
namespace phx::world
{
	struct Component
	{
		PHX_REFLECT_TYPE();
	};

	struct TranslationComponent :public Component
	{
		hlslpp::float3 translation = { 0.0f, 0.0f, 0.0f };
		hlslpp::float4 rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
		hlslpp::float3 scale = { 0.0f, 0.0f, 0.0f };

		PHX_REFLECT_TYPE();
	};

	struct LevelBlueprint
	{
		data::String name;
		data::DataPtr<Component> component;
		float test_data;

		PHX_REFLECT_TYPE();

	};
}