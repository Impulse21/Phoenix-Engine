#pragma once

#include <vector>
#include <memory>

namespace PhxEngine
{
	class AppLayer;

	class LayerStack final
	{
	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer(std::shared_ptr<AppLayer> layer);
		void PushOverlay(std::shared_ptr<AppLayer> layer);

		void PopLayer(std::shared_ptr<AppLayer> layer);
		void PopOverlay(std::shared_ptr<AppLayer> layer);

		std::vector<std::shared_ptr<AppLayer>>::iterator begin() { return this->m_layers.begin(); }
		std::vector<std::shared_ptr<AppLayer>>::iterator end() { return this->m_layers.end(); }
		std::vector<std::shared_ptr<AppLayer>>::reverse_iterator rbegin() { return this->m_layers.rbegin(); }
		std::vector<std::shared_ptr<AppLayer>>::reverse_iterator rend() { return this->m_layers.rend(); }

		std::vector<std::shared_ptr<AppLayer>>::const_iterator begin() const { return this->m_layers.begin(); }
		std::vector<std::shared_ptr<AppLayer>>::const_iterator end()	const { return this->m_layers.end(); }
		std::vector<std::shared_ptr<AppLayer>>::const_reverse_iterator rbegin() const { return this->m_layers.rbegin(); }
		std::vector<std::shared_ptr<AppLayer>>::const_reverse_iterator rend() const { return this->m_layers.rend(); }

	private:
		std::vector<std::shared_ptr<AppLayer>> m_layers;
		size_t m_layerInserIndex = 0;
	};
}

