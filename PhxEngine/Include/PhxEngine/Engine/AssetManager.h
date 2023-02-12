#pragma once


namespace PhxEngine
{
	class AssetManager
	{
	public:
		inline static AssetManager* GPtr = nullptr;

		void RegisterPath(std::string const& path);


	private:
		std::vector<std::string> m_registeredPaths;
	};
}

