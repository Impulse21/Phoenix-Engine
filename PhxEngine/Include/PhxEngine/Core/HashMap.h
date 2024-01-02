#pragma once

#include <limits>
#include <PhxEngine/Core/Memory.h>
#include <unordered_dense.h>
#include <mimalloc.h>
// Raptor Engine
namespace PhxEngine::Core
{
	template<typename K, typename V>
	using unordered_map = ankerl::unordered_dense::map<K, V, hash<K>, std::equal_to<K>, mi_stl_allocator<T>>;
	template<typename K, typename V>
	using unordered_segmented_map = ankerl::unordered_dense::segmented_map<K, V, hash<K>, std::equal_to<K>, mi_stl_allocator<T>>;
	template<typename K, typename V>
	using unordered_set = ankerl::unordered_dense::set<V, hash<K>, std::equal_to<K>, mi_stl_allocator<T>>;
	template<typename K, typename V>
	using unordered_segmented_set = ankerl::unordered_dense::segmented_set<V, hash<K>, std::equal_to<K>, mi_stl_allocator<T>>;
}