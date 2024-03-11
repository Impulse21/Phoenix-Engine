#include "PhxEngine/Scene/Entity.h"
#include <DirectXMath.h>

using namespace DirectX;
using namespace PhxEngine;

Entity::Entity(entt::entity handle, Scene* scene)
	: m_entityHandle(handle)
	, m_scene(scene)
{
}
