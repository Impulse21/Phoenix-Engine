
include(../../PhxEngine/compileshaders.cmake)

file(GLOB shaders "*.hlsl")
file(GLOB sources "*.cpp" "*.h")

set(project VisibilityBufferRendering)
set(folder "Examples/VisibilityBufferRendering")

message("Fetching DirectXShaderCompiler")
FetchContent_Declare(dxc URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2207/dxc_2022_07_18.zip)
FetchContent_MakeAvailable(dxc)

add_executable(${project} WIN32 ${sources})
target_link_libraries(${project} PhxEngine)
set_target_properties(${project} PROPERTIES FOLDER ${folder})

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()