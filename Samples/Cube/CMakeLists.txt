
add_executable(PhxCubeApp 
    "CubeApp.cpp"
)

target_link_libraries(PhxCubeApp PRIVATE PhxEngine)


target_include_directories(PhxCubeApp PRIVATE
    "${CMAKE_SOURCE_DIR}/Engine/Include"
)

set_target_properties(PhxCubeApp PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "PhxCubeApp"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)