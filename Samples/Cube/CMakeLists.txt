
add_executable(PhxCubeApp
    "CubeApp.cpp"
)

phx_add_app(PhxCubeApp)

target_compile_definitions(PhxCubeApp PRIVATE
    PHX_SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
)