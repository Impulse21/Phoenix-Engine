
add_executable(PhxModelViewer
    "ModelViewerApp.cpp"
)

phx_add_app(PhxModelViewer)

target_compile_definitions(PhxModelViewer PRIVATE
    # Reuses Cube's shader (matches ModelViewerApp.cpp's existing reuse of
    # Cube's GpuVertex/pipeline setup) -- Samples/ModelViewer/Shaders/ is
    # empty, so this was never previously reachable.
    PHX_SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../Cube/Shaders"
    PHX_ASSET_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets"
)