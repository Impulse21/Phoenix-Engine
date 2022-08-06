#pragma once

#include <vector>

namespace PhxEngine
{
	class AppLayer;

	class LayerStack final
	{
	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer(AppLayer* layer);
		void PushOverlay(AppLayer* layer);

		void PopLayer(AppLayer* layer);
		void PopOverlay(AppLayer* layer);

		std::vector<AppLayer*>::iterator begin() { return this->m_layers.begin(); }
		std::vector<AppLayer*>::iterator end() { return this->m_layers.end(); }
		std::vector<AppLayer*>::reverse_iterator rbegin() { return this->m_layers.rbegin(); }
		std::vector<AppLayer*>::reverse_iterator rend() { return this->m_layers.rend(); }

		std::vector<AppLayer*>::const_iterator begin() const { return this->m_layers.begin(); }
		std::vector<AppLayer*>::const_iterator end()	const { return this->m_layers.end(); }
		std::vector<AppLayer*>::const_reverse_iterator rbegin() const { return this->m_layers.rbegin(); }
		std::vector<AppLayer*>::const_reverse_iterator rend() const { return this->m_layers.rend(); }

	private:
		std::vector<AppLayer*> m_layers;
		size_t m_layerInserIndex = 0;
	};
}

