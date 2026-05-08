file(GLOB_RECURSE CUBE_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(PhxCubeApp ${CUBE_SOURCES}

target_link_libraries(PhxCubeApp PRIVATE libPhxEngine)

set_target_properties(PhxCubeApp PROPERTIES
    OUTPUT_NAME "PhxCubeApp"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)