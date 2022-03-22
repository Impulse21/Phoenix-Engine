#pragma once

#include <PhxEngine/Core/Asserts.h>

#include <stdint.h>
#include <atomic>
#include <vector>
#include <unordered_map>

namespace PhxEngine::ECS
{
	// Entity definition
	using Entity = uint32_t;
	static const Entity InvalidEntity = 0;

	inline Entity CreateEntity()
	{
		static std::atomic<Entity> next = {InvalidEntity + 1};
		return next.fetch_add(1);
	}

	template<typename TComponent>
	class ComponentStore
	{
	public:
		ComponentStore(size_t reservedCount = 0)
		{
			this->m_entities.reserve(reservedCount);
			this->m_components.reserve(reservedCount);
			this->m_lookup.reserve(reservedCount);
		}

		bool Contains(Entity entity)
		{
			return this->m_lookup.find(entity) != this->m_lookup.end();
		}

		TComponent& Create(Entity entity)
		{
			PHX_ASSERT(entity != InvalidEntity);
			PHX_ASSERT(!this->Contains(entity));

			PHX_ASSERT(this->m_entities.size() == this->m_components.size());
			PHX_ASSERT(this->m_lookup.size() == this->m_components.size());

			this->m_lookup[entity] = this->m_components.size();

			this->m_components.emplace_back();
			this->m_entities.push_back(entity);

			return this->m_components.back();
		}

		void Remove(Entity entity)
		{
			auto it = this->m_lookup.find(entity);
			if (it == this->m_lookup.end())
			{
				return;
			}

			const auto index = it->second;
			const Entity entityToRemove = this->m_entities[index];

			if (index < this->m_components.size() - 1)
			{
				this->m_components[index] = std::move(this->m_components.back());
				this->m_entities[index] = this->m_entities.back();

				this->m_lookup[this->m_entities[index]] = index;
			}

			this->m_components.pop_back();
			this->m_entities.pop_back();
			this->m_lookup.erase(entityToRemove);
		}

		void Remove_KeepSorted(Entity entity)
		{
			auto it = this->m_lookup.find(entity);
			if (it == this->m_lookup.end())
			{
				return;
			}

			const auto index = it->second;
			const Entity entityToRemove = this->m_entities[index];

			if (index < this->m_components.size() - 1)
			{
				for (size_t i = index + 1; i < this->m_components.size(); i++)
				{
					this->m_components[i - 1] = std::move(this->m_components[i]);
				}

				for (size_t i = index + 1; i < this->m_components.size(); i++)
				{
					this->m_entities[i - 1] = this->m_entities[i];
					this->m_lookup[this->m_entities[i - 1]] = i - 1;
				}
			}

			this->m_components.pop_back();
			this->m_entities.pop_back();
			this->m_lookup.erase(entityToRemove);
		}

		void MoveItem(size_t indexFrom, size_t indexTo)
		{
			PHX_ASSERT(indexFrom < this->GetCount());
			PHX_ASSERT(indexTo < this->GetCount());
			if (indexFrom == indexTo)
			{
				return;
			}

			// Store Component we are moving
			TComponent component = std::move(this->m_components[indexFrom]);
			Entity entity = this->m_entities[indexFrom];

			const int direction = indexFrom < indexTo ? 1 : -1;
			for (size_t i = indexFrom; i != indexTo; i += direction)
			{
				const size_t next = i + direction;
				this->m_components[i] = std::move(this->m_components[next]);
				this->m_entities[i] = this->m_entities[next];
				this->m_lookup[this->m_entities[i]] = i;
			}

			this->m_components[indexTo] = std::move(component);
			this->m_entities[indexTo] = entity;
			this->m_lookup[entity] = indexTo;
		}

		inline TComponent* GetComponent(Entity entity)
		{
			auto it = this->m_lookup.find(entity);
			if (it == this->m_lookup.end())
			{
				return nullptr;
			}

			return &this->m_components[it->second];
		}


		inline const TComponent* GetComponent(Entity entity) const
		{
			auto it = this->m_lookup.find(entity);
			if (it == this->m_lookup.end())
			{
				return nullptr;
			}

			return &this->m_components[it->second];
		}

		inline size_t GetIndex(Entity entity) const
		{
			auto it = this->m_lookup.find(entity);
			if (it == this->m_lookup.end())
			{
				return ~0u;
			}

			return it->second;
		}

		inline size_t GetCount() const { return this->m_components.size(); }
		inline bool IsEmpty() const { return this->GetCount() == 0; }
		inline Entity GetEntity(size_t index) const { return this->m_entities[index]; }

		inline TComponent& operator[](size_t index) { return this->m_components[index]; }
		inline const TComponent& operator[](size_t index) const { return this->m_components[index]; }

	private:
		std::vector<TComponent> m_components;
		std::vector<Entity> m_entities;
		std::unordered_map<Entity, size_t> m_lookup;


		// Disallow this to be copied by mistake
		ComponentStore(const ComponentStore&) = delete;
	};
}

