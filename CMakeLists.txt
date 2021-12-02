cmake_minimum_required(VERSION 3.13)

# set the project name and version
project(PhxEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set_property( GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")

include(CMakeDependentOption)

add_subdirectory(ThirdParty)

include(PhxEngineCore.cmake)
include(PhxEngineRHI.cmake)
include(PhxEngineApp.cmake)

# -- Add Apps ---
add_subdirectory(TestBedApp)