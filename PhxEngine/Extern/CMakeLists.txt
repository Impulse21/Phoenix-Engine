cmake_minimum_required(VERSION 3.16)


add_subdirectory(optick)
set_target_properties(OptickCore PROPERTIES FOLDER ${extern_folder})

file (GLOB nlohmann_json_src
    ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json/*.hpp)

add_library(json INTERFACE ${nlohmann_json_src})
target_include_directories(json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann_json" )
set_target_properties(json PROPERTIES FOLDER ${extern_folder})