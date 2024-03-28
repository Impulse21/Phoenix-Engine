#include "PhxEngine/World/Entity.h"
#include <DirectXMath.h>

using namespace DirectX;
using namespace PhxEngine;

Entity::Entity(entt::entity handle, World* world)
	: m_entityHandle(handle)
	, m_world(world)
{
}
