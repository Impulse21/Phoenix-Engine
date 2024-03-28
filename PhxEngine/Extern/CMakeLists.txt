cmake_minimum_required(VERSION 3.16)


add_subdirectory(optick)
set_target_properties(OptickCore PROPERTIES FOLDER ${extern_folder})

file (GLOB nlohmann_json_src
    ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json/*.hpp)

add_library(json INTERFACE ${nlohmann_json_src})
target_include_directories(json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json" )
set_target_properties(json PROPERTIES FOLDER ${extern_folder})


# GLFW Library
option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
add_subdirectory(glfw)

set_target_properties(glfw PROPERTIES FOLDER ${extern_folder})
set_target_properties(update_mappings PROPERTIES FOLDER ${extern_folder})

add_subdirectory(D3D12MemoryAllocator)
set_target_properties(D3D12MemoryAllocator PROPERTIES FOLDER ${extern_folder})
 target_include_directories(D3D12MemoryAllocator INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/D3D12MemoryAllocator/include)

 
add_subdirectory(DirectX-Headers)
set_target_properties(DirectX-Guids PROPERTIES FOLDER ${extern_folder})

add_library(entt INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/entt/entt.hpp)
target_include_directories(entt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/entt)
set_target_properties(entt PROPERTIES FOLDER ${extern_folder})


add_library(taskflow INTERFACE)
target_include_directories(taskflow INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/taskflow)