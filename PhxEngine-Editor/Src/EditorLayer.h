#pragma once

#include <PhxEngine/App/Layer.h>

class EditorLayer : public PhxEngine::AppLayer
{
public:
	EditorLayer()
		: AppLayer("Editor Layer")
	{};

	void OnRenderImGui() override;
};

