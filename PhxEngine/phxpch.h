#pragma once


#ifdef _WIN32

#ifndef NOMINMAX
	// See github.com/skypjack/entt/wiki/Frequently-Asked-Questions#warning-c4003-the-min-the-max-and-the-macro
	#define NOMINMAX
#endif

	#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#endif

// -- STL Includes ---
#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <assert.h>

// -- Window Includes ---
#ifdef _WIN32
	#include <Windows.h>
#endif 
