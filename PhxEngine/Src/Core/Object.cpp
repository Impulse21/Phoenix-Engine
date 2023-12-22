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

bool PhxEngine::Core::Object::IsInstanceOf(StringHash type) const
{
	return this->GetTypeInfo()->IsTypeOf(type);
}

bool PhxEngine::Core::Object::IsInstanceOf(const TypeInfo* typeInfo) const
{
	return this->GetTypeInfo()->IsTypeOf(typeInfo);
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
	if (!object)
		return {};

	ScopedSpinLock _(m_spinLock);
	ObjectIdImpl impl = {};
	impl.ObjectPtr = object;

	return m_trackedObjects.Insert(impl);
}
void ObjectTracker::RemoveInstance(Handle<ObjectId> handle)
{
	if (!handle.IsValid())
		return;

	ScopedSpinLock _(m_spinLock);
	m_trackedObjects.Release(handle);
}

void ObjectTracker::Finalize()
{
	ScopedSpinLock _(m_spinLock);
	if (!m_trackedObjects.IsEmpty())
	{
		PHX_LOG_CORE_ERROR("Memory Leak Detected: Objects are still active.");
	}

	m_trackedObjects.Finalize();
}

PhxEngine::Core::TypeInfo::TypeInfo(const char* typeName, const TypeInfo* baseTypeInfo)
	: m_type(typeName)
	, m_typeName(typeName)
	, m_baseTypeInfo(baseTypeInfo)
{
}

bool PhxEngine::Core::TypeInfo::IsTypeOf(StringHash type) const
{
	const TypeInfo* current = this;
	while (current)
	{
		if (current->GetType() == type)
			return true;

		current = current->GetBaseTypeInfo();
	}

	return false;
}

bool PhxEngine::Core::TypeInfo::IsTypeOf(const TypeInfo* typeInfo) const
{
	if (typeInfo == nullptr)
		return false;

	const TypeInfo* current = this;
	while (current)
	{
		if (current == typeInfo || current->GetType() == typeInfo->GetType())
			return true;

		current = current->GetBaseTypeInfo();
	}

	return false;
}
