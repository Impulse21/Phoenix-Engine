#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"

#include "PhxEngine/Scene/Entity.h"

using namespace PhxEngine::Scene;

Entity::Entity(entt::entity handle, New::Scene* scene)
	: m_entityHandle(handle)
	, m_scene(scene)
{
}
