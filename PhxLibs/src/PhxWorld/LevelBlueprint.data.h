#pragma once

#include <PhxReflection/Reflection.h>
#include <PhxData/DataContainers.h>
#include <PhxData/DataPtr.h>

#include <hlsl++.h>
namespace phx::world
{
	struct Component
	{
		uint64_t type;
		PHX_REFLECTION_VARS();
		PHX_DEFINE_REFLECTION()
		{
			phx::reflection::Reflect<Component>("Component")
				.Register();
		}
	};

	struct TranslationComponent :public Component
	{
		hlslpp::float3 translation = { 0.0f, 0.0f, 0.0f };
		hlslpp::float4 rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
		hlslpp::float3 scale = { 0.0f, 0.0f, 0.0f };

		PHX_REFLECTION_VARS();
		PHX_DEFINE_REFLECTION()
		{
			phx::reflection::Reflect<phx::world::TranslationComponent>("TranslationComponent")
				.Parent<phx::world::Component>()
				.Property<hlslpp::float3, &phx::world::TranslationComponent::translation>("translation")
				.Property<hlslpp::float4, &phx::world::TranslationComponent::rotation>("rotation")
				.Property<hlslpp::float3, &phx::world::TranslationComponent::scale>("scale")
				.Register();
		}
	};

	struct LevelBlueprint
	{
		data::String name;
		data::DataPtr<Component> component;
		float test_data;

		PHX_REFLECTION_VARS();
		PHX_DEFINE_REFLECTION()
		{
			phx::reflection::Reflect<phx::world::LevelBlueprint>("LevelBlueprint")
				.Property<phx::data::String, &phx::world::LevelBlueprint::name>("name")
				.Property<phx::data::DataPtr<phx::world::Component>, &phx::world::LevelBlueprint::component>("components")
				.Register();
		}

	};
}