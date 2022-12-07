#pragma once

#include <DirectXMath.h>
#include <Directxpackedvector.h>

#include <vector>
#include <algorithm>
#include <atomic>

namespace PhxEngine::Renderer
{
	// From WICKED Engine - thought this was a really cool set of classes.
	// https://github.com/turanszkij/WickedEngine
	struct DrawBatch
	{
		uint64_t data;
		inline void Create(uint32_t meshEntityHandle, uint32_t instanceEntityHandle, float distance)
		{
			// These asserts are a indicating if render queue limits are reached:
			assert(meshEntityHandle < 0x00FFFFFF);
			assert(instanceEntityHandle < 0x00FFFFFF);

			data = 0;
			data |= uint64_t(meshEntityHandle & 0x00FFFFFF) << 40ull;
			data |= uint64_t(DirectX::PackedVector::XMConvertFloatToHalf(distance) & 0xFFFF) << 24ull;
			data |= uint64_t(instanceEntityHandle & 0x00FFFFFF) << 0ull;
		}

		inline float GetDistance() const
		{
			return DirectX::PackedVector::XMConvertHalfToFloat(DirectX::PackedVector::HALF((data >> 24ull) & 0xFFFF));
		}
		inline uint32_t GetMeshEntityHandle() const
		{
			return (data >> 40ull) & 0x00FFFFFF;
		}
		inline uint32_t GetInstanceEntityHandle() const
		{
			return (data >> 0ull) & 0x00FFFFFF;
		}

		// opaque sorting
		//	Priority is set to mesh index to have more instancing
		//	distance is second priority (front to back Z-buffering)
		bool operator<(const DrawBatch& other) const
		{
			return data < other.data;
		}
		// transparent sorting
		//	Priority is distance for correct alpha blending (back to front rendering)
		//	mesh index is second priority for instancing
		bool operator>(const DrawBatch& other) const
		{
			// Swap bits of meshIndex and distance to prioritize distance more
			uint64_t a_data = 0ull;
			a_data |= ((data >> 24ull) & 0xFFFF) << 48ull; // distance repack
			a_data |= ((data >> 40ull) & 0x00FFFFFF) << 24ull; // meshIndex repack
			a_data |= data & 0x00FFFFFF; // instanceIndex repack
			uint64_t b_data = 0ull;
			b_data |= ((other.data >> 24ull) & 0xFFFF) << 48ull; // distance repack
			b_data |= ((other.data >> 40ull) & 0x00FFFFFF) << 24ull; // meshIndex repack
			b_data |= other.data & 0x00FFFFFF; // instanceIndex repack
			return a_data > b_data;
		}
	};

	struct DrawQueue
	{
		// TODO: Allocate from a frame memory
		std::vector<DrawBatch> DrawBatches;

		inline void Push(uint32_t meshEntityHandle, uint32_t instanceEntityHandle, float distance)
		{
			DrawBatches.emplace_back().Create(meshEntityHandle, instanceEntityHandle, distance);
		}

		inline void SortTransparent()
		{
			std::sort(DrawBatches.begin(), DrawBatches.end(), std::greater<DrawBatch>());
		}

		inline void SortOpaque()
		{
			std::sort(DrawBatches.begin(), DrawBatches.end(), std::less<DrawBatch>());
		}

		inline void Reset()
		{
			DrawBatches.clear();
		}

		inline bool Empty() const
		{
			return DrawBatches.empty();
		}

		inline size_t Size() const
		{
			return DrawBatches.size();
		}
	};
}

