add_executable(PhxHorde
    "HordeApp.cpp"
)

phx_add_app(PhxHorde)

target_compile_definitions(PhxHorde PRIVATE
    PHX_SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
    PHX_ASSET_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets"
)
