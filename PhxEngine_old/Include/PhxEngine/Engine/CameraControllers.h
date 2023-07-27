#pragma

#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Core/TimeStep.h>


namespace PhxEngine
{
	namespace Core
	{
		class IWindow;
	}

	class FirstPersonCameraController
	{
	public:
		FirstPersonCameraController() = default;
		~FirstPersonCameraController() = default;

		void OnUpdate(
			Core::IWindow* window,
			PhxEngine::Core::TimeStep const& timestep,
			PhxEngine::Scene::CameraComponent& camera);

	private:
		void Walk(float d, PhxEngine::Scene::CameraComponent& camera);
		void Strafe(float d, PhxEngine::Scene::CameraComponent& camera);
		void Pitch(float angle, PhxEngine::Scene::CameraComponent& camera);
		void RotateY(float angle, PhxEngine::Scene::CameraComponent& camera);


	};
}