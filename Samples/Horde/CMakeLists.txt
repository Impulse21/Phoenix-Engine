add_executable(PhxHorde
    "HordeApp.cpp"
    "HordeLevelFactory.cpp"
    "HordeRenderer.cpp"
)

phx_add_app(PhxHorde)

# -- Add dependencies ---
target_link_libraries(PhxHorde PRIVATE stb::stb)

target_compile_definitions(PhxHorde PRIVATE
    PHX_SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Shaders"
    PHX_ASSET_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets"
    PHX_ASSET_CORE_SOURCE_DIR="${CMAKE_SOURCE_DIR}/Assets"
)


