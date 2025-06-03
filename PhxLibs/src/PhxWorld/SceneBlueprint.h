#pragma once

#include <PhxResource/IResource.h>
#include <PhxCore/Containers/Array.h>
namespace phx
{
	struct Node
	{

	};


	struct SceneBlueprint : public Resource
	{
		phx::Array<Node> Nodes;
		Node* Root = nullptr;

		~SceneBlueprint()
		{
			Nodes.Shutdown();
		}
	};
}