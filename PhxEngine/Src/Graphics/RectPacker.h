#pragma once

#include <vector>

#include "ThirdParty/Stb/stb_rect_pack.h"

namespace PhxEngine::Graphics
{
	using PackerRect = stbrp_rect;

	class RectPacker
	{
	public:
		void AddRect(PackerRect const& rect)
		{
			this->m_rects.push_back(rect);
			this->m_width = std::max(rect.w, this->m_width);
			this->m_height = std::max(rect.h, this->m_height);
		}

		void Clear()
		{
			this->m_rects.clear();
		}

		bool IsEmpty() const { return this->m_rects.empty(); }
		const std::vector<PackerRect>& GetRects() const { return this->m_rects; }

		bool Pack(int maxWidth)
		{
			while (this->m_width <= maxWidth || this->m_height <= maxWidth)
			{
				if (this->m_nodes.size() < this->m_width)
				{
					this->m_nodes.resize(this->m_width);
				}
				stbrp_init_target(&this->m_context, this->m_width, this->m_height, this->m_nodes.data(), int(this->m_nodes.size()));
				if (stbrp_pack_rects(&this->m_context, this->m_rects.data(), int(this->m_rects.size())))
				{
					return true;
				}

				// If there isn't enough room, double the size and try again.
				if (this->m_height < this->m_width)
				{
					this->m_height *= 2;
				}
				else
				{
					this->m_width *= 2;
				}
			}

			this->m_width = 0;
			this->m_height = 0;
			return false;
		}

		int GetWidth() const { return this->m_width; }
		int GetHeight() const { return this->m_height; }

	private:
		stbrp_context m_context = {};
		std::vector<PackerRect> m_rects;
		std::vector<stbrp_node> m_nodes;
		int m_width = 0;
		int m_height = 0;
	};
}