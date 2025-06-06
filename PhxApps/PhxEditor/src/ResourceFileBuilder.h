#pragma once

#include "CompiledResource.h"
#include <memory>

namespace phx
{
	class IBlob;
}
namespace phxed
{
	class ResourceFileBuilder
	{
	public:
		static std::unique_ptr<phx::IBlob> Build(CompiledResource* resource)
		{
			ResourceFileBuilder builder(resource);
			return builder.Build();
		}

	public:
		ResourceFileBuilder(CompiledResource* resource)
			: m_resource(resource)

		{}

		std::unique_ptr<phx::IBlob> Build();

	private:
		CompiledResource* m_resource;
	};
}

