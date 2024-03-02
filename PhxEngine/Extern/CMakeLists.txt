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
