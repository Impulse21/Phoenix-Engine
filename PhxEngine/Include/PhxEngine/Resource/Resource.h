#pragma once

#include <PhxEngine/Core/RefCountPtr.h>

namespace PhxEngine
{
	class Resource : public RefCounted
	{
		PHX_OBJECT(Resource, RefCounted);

	public:
		Resource() = default;
		virtual ~Resource() = default;

	};
}

