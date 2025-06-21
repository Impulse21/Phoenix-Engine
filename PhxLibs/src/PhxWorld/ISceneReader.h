#pragma once

#include <PhxCore/Span.h>
#include <variant>

#include "WorldMetadata.def.h"

namespace phx
{
	struct ComponentDescriptor
	{
		StringHash type_hash;
		std::variant<void*, std::string> data;
	};

	class ISceneReader
	{
	public:
		virtual ~ISceneReader() = default;

		virtual size_t GetNodeCount() const = 0;
		virtual const phx::Transform& GetTransform(size_t scene_node) = 0;
		virtual const std::string& GetName(size_t scene_node) = 0;

		virtual Span<ComponentDescriptor> GetComponentDescriptors(size_t scene_node) = 0;

	};
}