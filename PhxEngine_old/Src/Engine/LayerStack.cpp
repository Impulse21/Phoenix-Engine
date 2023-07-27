#include "phxpch.h"
#include "PhxEngine/Engine/LayerStack.h"

#include "PhxEngine/Engine/Layer.h"

PhxEngine::LayerStack::~LayerStack()
{
	for (auto& layer : this->m_layers)
	{
		layer->OnDetach();
	}
}

void PhxEngine::LayerStack::PushLayer(std::shared_ptr<AppLayer> layer)
{
	this->m_layers.emplace(this->m_layers.begin() + this->m_layerInserIndex, layer);
	this->m_layerInserIndex++;
	layer->OnAttach();
}

void PhxEngine::LayerStack::PushOverlay(std::shared_ptr<AppLayer> layer)
{
	this->m_layers.push_back(layer);
	layer->OnAttach();
}

void PhxEngine::LayerStack::PopLayer(std::shared_ptr<AppLayer> layer)
{
	auto itr = std::find(this->m_layers.begin(), this->m_layers.begin() + this->m_layerInserIndex, layer);
	if (itr != this->m_layers.begin() + this->m_layerInserIndex)
	{
		layer->OnDetach();
		this->m_layers.erase(itr);
		this->m_layerInserIndex--;
	}
}

void PhxEngine::LayerStack::PopOverlay(std::shared_ptr<AppLayer> layer)
{
	auto itr = std::find(this->m_layers.begin() + this->m_layerInserIndex, this->m_layers.end() , layer);
	if (itr != this->m_layers.end())
	{
		layer->OnDetach();
		this->m_layers.erase(itr);
	}
}

