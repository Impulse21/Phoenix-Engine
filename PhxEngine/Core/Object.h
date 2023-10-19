#pragma once

#include "Handle.h"
#include <Core/Memory.h>
#include <Core/RefCountPtr.h>

namespace PhxEngine::Core
{
	struct ObjectId;

	class Object
	{
	public:
		Object();
		virtual ~Object();
	private:
		Core::Handle<ObjectId> m_trackerId;
	};

	namespace ObjectTracker
	{
		Core::Handle<ObjectId> RegisterInstance(Object* object);
		void RemoveInstance(Core::Handle<ObjectId> handle);

		void Finalize();
	}
}

