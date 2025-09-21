#pragma once
#include <cstdint>

// Credit to: https://github.dev/FireFlyForLife/NeatReflection
namespace phx::reflection
{
	using TemplateTypeId = uint32_t;
	inline constexpr TemplateTypeId kEmptyTypeId = 0;


	TemplateTypeId GenerateNewTypeId();

	template<typename T>
	TemplateTypeId GetId()
	{
		static const TemplateTypeId s_id = GenerateNewTypeId();
		return s_id;	
	}

	// Manual type id override
	template<typename T>
	concept ManuallyDefinedTemplateTypeId = requires { typename T::ManualId; };

	template<ManuallyDefinedTemplateTypeId T>
	constexpr TemplateTypeId getId()
	{
		return T::ManualId::value;
	}
}

