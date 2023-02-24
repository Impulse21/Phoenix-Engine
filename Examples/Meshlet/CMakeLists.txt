
include(../../PhxEngine/compileshaders.cmake)

file(GLOB shaders "*.hlsl")
file(GLOB sources "*.cpp" "*.h")

set(project Meshlet)
set(folder "Examples/Meshlet")


set_source_files_properties(BasicTriangleAS.hlsl PROPERTIES ShaderType "as" ShaderModel "6_6" EntryPoint "main")
set_source_files_properties(BasicTriangleMS.hlsl PROPERTIES ShaderType "ms" ShaderModel "6_6" EntryPoint "main")
set_source_files_properties(BasicTrianglePS.hlsl PROPERTIES ShaderType "ps" ShaderModel "6_6" EntryPoint "main")

message("Fetching DirectXShaderCompiler")
FetchContent_Declare(dxc URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2207/dxc_2022_07_18.zip)
FetchContent_MakeAvailable(dxc)

dxc_compile_shaders(
    TARGET ${project}_shaders
    SOURCES ${shaders}
    CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/shaders.cfg
    FOLDER ${folder}
    OUTPUT_BASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Shaders/${project}
)

add_executable(${project} WIN32 ${sources})
target_link_libraries(${project} PhxEngine)
add_dependencies(${project} ${project}_shaders)
set_target_properties(${project} PROPERTIES FOLDER ${folder})

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()