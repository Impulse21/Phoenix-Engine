
add_executable(PhxCubeApp
    "CubeApp.cpp"
)

phx_add_app(PhxCubeApp)


target_link_libraries(PhxCubeApp PRIVATE stb::stb)

target_compile_definitions(PhxCubeApp PRIVATE
    PHX_SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
    PHX_ASSET_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets"
)