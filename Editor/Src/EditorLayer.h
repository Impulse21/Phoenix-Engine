
#include <PhxEngine/Core/Layer.h>

class EditorLayer : public PhxEngine::Core::Layer
{
public:
	EditorLayer() = default;
	~EditorLayer() = default;

	void OnImGuiRender() override;
};
