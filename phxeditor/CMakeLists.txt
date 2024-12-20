# Required dependencies

file(GLOB src_editor
    src/*.h
    src/*.cpp)

file(GLOB src_editor
    src/*.h
    src/*.cpp)


add_executable(PhxEditor ${src_editor})
target_link_libraries(PhxEditor PhxEngine)

set_target_properties(PhxEditor  PROPERTIES FOLDER "Applications")
set_target_properties(PhxEditor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
target_compile_options(PhxEditor PRIVATE ${COMMON_COMPILE_OPTIONS})

# Add shader files to the project as resources
file(GLOB shaders_editor
    Shaders/*.hlsli
    Shaders/*.hlsl
)

# Optionally organize them in the Visual Studio Solution Explorer
source_group("Shaders/Editor" FILES ${shaders_editor})
