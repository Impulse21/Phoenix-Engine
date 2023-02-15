#pragma once

#include <PhxEngine/RHI/PhxRHI.h>

#include <PhxEngine/Shaders/ShaderInteropStructures.h>

namespace PhxEngine::Scene
{
	class Scene;
}

namespace PhxEngine::Renderer
{	
	// From WICKED Engine - thought this was a really cool set of classes.
	// https://github.com/turanszkij/WickedEngine
	struct DrawItem
	{
		uint64_t Data;

		inline void Create(uint32_t meshEntityHandle, uint32_t instanceEntityHandle, float distance)
		{
			// These asserts are a indicating if render queue limits are reached:
			assert(meshEntityHandle < 0x00FFFFFF);
			assert(instanceEntityHandle < 0x00FFFFFF);

			Data = 0;
			Data |= uint64_t(meshEntityHandle & 0x00FFFFFF) << 40ull;
			Data |= uint64_t(DirectX::PackedVector::XMConvertFloatToHalf(distance) & 0xFFFF) << 24ull;
			Data |= uint64_t(instanceEntityHandle & 0x00FFFFFF) << 0ull;
		}

		inline float GetDistance() const
		{
			return DirectX::PackedVector::XMConvertHalfToFloat(DirectX::PackedVector::HALF((Data >> 24ull) & 0xFFFF));
		}
		inline uint32_t GetMeshEntityHandle() const
		{
			return (Data >> 40ull) & 0x00FFFFFF;
		}
		inline uint32_t GetInstanceEntityHandle() const
		{
			return (Data >> 0ull) & 0x00FFFFFF;
		}

		// opaque sorting
		//	Priority is set to mesh index to have more instancing
		//	distance is second priority (front to back Z-buffering)
		bool operator<(const DrawItem& other) const
		{
			return Data < other.Data;
		}
		// transparent sorting
		//	Priority is distance for correct alpha blending (back to front rendering)
		//	mesh index is second priority for instancing
		bool operator>(const DrawItem& other) const
		{
			// Swap bits of meshIndex and distance to prioritize distance more
			uint64_t a_data = 0ull;
			a_data |= ((Data >> 24ull) & 0xFFFF) << 48ull; // distance repack
			a_data |= ((Data >> 40ull) & 0x00FFFFFF) << 24ull; // meshIndex repack
			a_data |= Data & 0x00FFFFFF; // instanceIndex repack
			uint64_t b_data = 0ull;
			b_data |= ((other.Data >> 24ull) & 0xFFFF) << 48ull; // distance repack
			b_data |= ((other.Data >> 40ull) & 0x00FFFFFF) << 24ull; // meshIndex repack
			b_data |= other.Data & 0x00FFFFFF; // instanceIndex repack
			return a_data > b_data;
		}
	};

	struct DrawQueue
	{
		// TODO: Allocate from a frame memory
		std::vector<DrawItem> DrawItems;

		inline void Push(uint32_t meshEntityHandle, uint32_t instanceEntityHandle, float distance)
		{
			DrawItems.emplace_back().Create(meshEntityHandle, instanceEntityHandle, distance);
		}

		inline void SortTransparent()
		{
			std::sort(DrawItems.begin(), DrawItems.end(), std::greater<DrawItem>());
		}

		inline void SortOpaque()
		{
			std::sort(DrawItems.begin(), DrawItems.end(), std::less<DrawItem>());
		}

		inline void Reset()
		{
			DrawItems.clear();
		}

		inline bool Empty() const
		{
			return DrawItems.empty();
		}

		inline size_t Size() const
		{
			return DrawItems.size();
		}
	};

	class IGeometryPass
	{
	public:
		virtual void BindPushConstant(RHI::ICommandList* commandList, Shader::GeometryPassPushConstants const& pushData) = 0;

	};

	void RenderViews(RHI::ICommandList* commandList, IGeometryPass* geometryPass, Scene::Scene& scene, DrawQueue& drawQueue, bool markMeshes = false);
}

