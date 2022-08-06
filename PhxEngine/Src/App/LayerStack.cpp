#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "LayerStack.h"

#include "Layer.h"

PhxEngine::LayerStack::~LayerStack()
{
	for (auto* layer : this->m_layers)
	{
		layer->OnDetach();
	}
}

void PhxEngine::LayerStack::PushLayer(AppLayer* layer)
{
	this->m_layers.emplace(this->m_layers.begin() + this->m_layerInserIndex, layer);
	this->m_layerInserIndex++;
	layer->OnAttach();
}

void PhxEngine::LayerStack::PushOverlay(AppLayer* layer)
{
	this->m_layers.push_back(layer);
	layer->OnAttach();
}

void PhxEngine::LayerStack::PopLayer(AppLayer* layer)
{
	auto itr = std::find(this->m_layers.begin(), this->m_layers.begin() + this->m_layerInserIndex, layer);
	if (itr != this->m_layers.begin() + this->m_layerInserIndex)
	{
		layer->OnDetach();
		this->m_layers.erase(itr);
		this->m_layerInserIndex--;
	}
}

void PhxEngine::LayerStack::PopOverlay(AppLayer* layer)
{
	auto itr = std::find(this->m_layers.begin() + this->m_layerInserIndex, this->m_layers.end() , layer);
	if (itr != this->m_layers.end())
	{
		layer->OnDetach();
		this->m_layers.erase(itr);
	}
}

