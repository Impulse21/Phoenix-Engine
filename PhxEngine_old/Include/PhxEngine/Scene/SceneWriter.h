#pragma once

#include <memory>
#include <string>

namespace PhxEngine::Scene
{
	namespace New
	{
		class Scene;
	}

	class ISceneWriter
	{
	public:
		static std::unique_ptr<ISceneWriter> Create();

		virtual ~ISceneWriter() = default;
		virtual bool Write(std::string const& filePath, Scene& scene) = 0;
	};

}

