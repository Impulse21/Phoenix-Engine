#include "PhxData/PhxData_pch.h"
#include "Reflection.h"


#include <unordered_map>
#include <vector>
#include <string>
#include <functional>


namespace
{
	struct TypeContainer
	{
		struct string_hash : std::hash<std::string_view>
		{
			using is_transparent = std::true_type;
		};

		std::unordered_map<std::string, uint32_t, string_hash, std::equal_to<>> ByTypeName;
		std::unordered_map<phx::data::TemplateTypeId, uint32_t> ByTemplateTypeId;
		std::vector<phx::data::TypeDescriptor> Types;
	};

	static TypeContainer g_TypeContainer;
}

namespace phx::data
{
	namespace TypeRegistry
	{

		TypeDescriptor& AddType(TypeDescriptor&& type)
		{
			auto type_by_id_it = g_TypeContainer.ByTemplateTypeId.find(type.Id);
			if (type_by_id_it != g_TypeContainer.ByTemplateTypeId.end())
			{
				return g_TypeContainer.Types[type_by_id_it->second];
			}

			g_TypeContainer.ByTypeName[type.Name] = g_TypeContainer.Types.size();
			g_TypeContainer.ByTemplateTypeId[type.Id] = g_TypeContainer.Types.size();
			g_TypeContainer.Types.push_back(std::move(type));
			return g_TypeContainer.Types.back();
		}

#if false
		phx::Span<const TypeDescriptor> GetTypes()
		{
			return { g_TypeContainer.Types.begin(), g_TypeContainer.Types.end() };
		}
#endif

		const TypeDescriptor* GetType(std::string_view typeName)
		{
			auto it = g_TypeContainer.ByTypeName.find(typeName);
			if (it != g_TypeContainer.ByTypeName.end())
			{
				return &g_TypeContainer.Types[it->second];
			}

			return nullptr;
		}

		const TypeDescriptor* GetType(TemplateTypeId typeId)
		{
			auto it = g_TypeContainer.ByTemplateTypeId.find(typeId);
			if (it != g_TypeContainer.ByTemplateTypeId.end())
			{
				return &g_TypeContainer.Types[it->second];
			}

			return nullptr;
		}

	}
}
