
add_executable(PhxModelViewer
    "ModelViewerApp.cpp"
)

phx_add_app(PhxModelViewer)

target_compile_definitions(PhxModelViewer PRIVATE
    PHX_SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
    PHX_ASSET_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets"
)