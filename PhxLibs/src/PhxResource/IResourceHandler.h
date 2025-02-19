#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>

namespace phx
{
	template<typename T>
	struct ResourceHandlerExtension;

	class IResource;
	class IResourceHandler
	{
	public:
		virtual RefCountPtr<IResource> Load(StringHash filename) = 0;

		virtual ~IResourceHandler() = default;
	};
}