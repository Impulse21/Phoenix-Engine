
cmake_minimum_required(VERSION 3.22)

# Group CMakePredefinedTargets into a folder filter
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set Windows SDK Version
set(CMAKE_SYSTEM_VERSION 10.0.19041.0)
project(PhxEngine)

# Output everything to Bin because some commands copies it to destination exe
# Not the best way, could get rid of .libs, but for now it'll do

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Output/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

include(CMakeDependentOption)
cmake_dependent_option(PHX_WITH_DX12 "Enable the DX12 version of Donut" ON "WIN32" OFF)

add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)
add_compile_definitions(NOMINMAX)

add_subdirectory(PhxEngine)
add_subdirectory(PhxEditor)
# add_subdirectory(Tests)
#add_subdirectory(Tools/PhxAssetConverter)
add_subdirectory(Tools/PhxShaderCompiler)