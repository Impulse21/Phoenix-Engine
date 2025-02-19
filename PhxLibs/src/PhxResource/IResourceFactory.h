#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/StringHash.h>

namespace phx
{
	template<typename T>
	struct ResourceFactoryExtension;

	class IResource;
	class IResourceFactory
	{
	public:
		virtual RefCountPtr<IResource> Create(StringHash filename, const char* name) = 0;

		virtual ~IResourceFactory() = default;
	};
}