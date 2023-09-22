#include <PhxEngine/Core/Object.h>
#include <PhxEngine/Core/Pool.h>
#include <PhxEngine/Core/SpinLock.h>
#include <PhxEngine/Core/Log.h>>

using namespace PhxEngine::Core;

PhxEngine::Core::Object::Object()
	: m_trackerId(ObjectTracker::RegisterInstance(this))
{
}

PhxEngine::Core::Object::~Object()
{
	if (this->m_trackerId.IsValid())
		ObjectTracker::RemoveInstance(this->m_trackerId);
}

namespace
{
	struct ObjectIdImpl
	{
		Object* ObjectPtr = nullptr;
	};
	Pool<ObjectIdImpl, ObjectId> m_trackedObjects;
	SpinLock m_spinLock;
}

using namespace PhxEngine::Core::ObjectTracker;

PhxEngine::Core::Handle<ObjectId> ObjectTracker::RegisterInstance(Object* object)
{
	return {};
	if (!object)
		return {};

	ScopedSpinLock _(m_spinLock);
	ObjectIdImpl impl = {};
	impl.ObjectPtr = object;

	return m_trackedObjects.Insert(impl);
}
void ObjectTracker::RemoveInstance(Handle<ObjectId> handle)
{
	return;
	if (!handle.IsValid())
		return;

	ScopedSpinLock _(m_spinLock);
	m_trackedObjects.Release(handle);
}

void ObjectTracker::CleanUp()
{
	ScopedSpinLock _(m_spinLock);
	if (!m_trackedObjects.IsEmpty())
		PHX_LOG_CORE_ERROR("Memory Leak Detected: Objects are still active.");
}

