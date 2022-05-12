#pragma once


namespace PhxEngine::Core
{
	struct CommandLineArgs
	{

	};

	class Application
	{
	public:
		// virtual void Initialize();
		//void RunFrame();

		//void FixedUpdate();
		//void Update();
		//void Render();

	private:

	};

	// Defined by client
	Application* CreateApplication(CommandLineArgs args);
}

