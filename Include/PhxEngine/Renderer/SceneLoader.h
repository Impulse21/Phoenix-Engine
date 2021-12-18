#pragma once

#include <string>

namespace PhxEngine::Renderer
{
	class Scene;

	class ISceneLoader
	{
	public:
		virtual Scene* LoadScene(std::string const& fileName) = 0;

		virtual ~ISceneLoader() = default;
	};

	ISceneLoader* CreateGltfSceneLoader();
}