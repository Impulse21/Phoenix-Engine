#pragma once

#include <PhxCore/RefCountPtr.h>
namespace phx
{
	template<typename T>
	struct ResourceFileExtension;

	template<typename T>
	struct ResourceFileHandlerId;


	struct Resource;
	class IResourceFileHandler
	{
	public:
		virtual ~IResourceFileHandler() = default;

		virtual RefCountPtr<Resource> LoadFromPak() const = 0;
		virtual RefCountPtr<Resource> LoadLoose(const char* filename) const = 0;

	};
}