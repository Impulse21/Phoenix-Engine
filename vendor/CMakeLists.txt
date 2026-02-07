# Vendor Libraries CMakeLists.txt

#==============================================================================
# ImGui
#==============================================================================
add_library(ImGui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(ImGui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(ImGui PUBLIC
    glfw
    Vulkan::Vulkan
)

# Visual Studio organization
set_target_properties(ImGui PROPERTIES FOLDER "_Dependencies")

# Apply source filters for Visual Studio
if(COMMAND apply_filters)
    apply_filters(ImGui)
endif()

#==============================================================================
# Tracy Profiler
#==============================================================================
add_library(Tracy STATIC
    ${tracy_SOURCE_DIR}/public/TracyClient.cpp
)


if(PLATFORM_WINDOWS)
    target_link_libraries(Tracy PUBLIC ws2_32 dbghelp)
elseif(PLATFORM_LINUX)
    target_link_libraries(Tracy PUBLIC pthread dl)
endif()

target_include_directories(Tracy PUBLIC
    ${tracy_SOURCE_DIR}/public
)

target_compile_definitions(Tracy PUBLIC
    TRACY_ENABLE
    TRACY_ON_DEMAND
)

#==============================================================================
# TLSF Allocator
#==============================================================================
add_library(tlsf STATIC
    ${tlsf_SOURCE_DIR}/tlsf.c
)

target_include_directories(tlsf PUBLIC
    ${tlsf_SOURCE_DIR}
)

# Visual Studio organization
set_target_properties(tlsf PROPERTIES FOLDER "_Dependencies")

# Apply source filters for Visual Studio
if(COMMAND apply_filters)
    apply_filters(tlsf)
endif()

#==============================================================================
# bc7enc_rdo
#==============================================================================
add_library(bc7enc_rdo STATIC
    ${bc7enc_rdo_SOURCE_DIR}/bc7enc.cpp
    ${bc7enc_rdo_SOURCE_DIR}/rgbcx.cpp
)

target_include_directories(bc7enc_rdo PUBLIC
    ${bc7enc_rdo_SOURCE_DIR}
)

# Visual Studio organization
set_target_properties(bc7enc_rdo PROPERTIES FOLDER "_Dependencies")

# Apply source filters for Visual Studio
if(COMMAND apply_filters)
    apply_filters(bc7enc_rdo)
endif()

#==============================================================================
# Header-Only Libraries (Interface targets)
#==============================================================================
# hlslpp
add_library(hlslpp INTERFACE)
target_include_directories(hlslpp INTERFACE
    ${hlslpp_SOURCE_DIR}/include
)

# stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE
    ${stb_SOURCE_DIR}
)

# cgltf
add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE
    ${cgltf_SOURCE_DIR}
)

# Slang Shader Compiler (pre-built binaries)
add_library(slang INTERFACE)
target_include_directories(slang INTERFACE
    ${slang_SOURCE_DIR}/include
)

if(PLATFORM_LINUX)
    target_link_directories(slang INTERFACE
        ${slang_SOURCE_DIR}/lib
    )
    target_link_libraries(slang INTERFACE
        ${slang_SOURCE_DIR}/lib/libgfx.so
        ${slang_SOURCE_DIR}/lib/libslang-compiler.so
        ${slang_SOURCE_DIR}/lib/libslang-rt.so
        ${slang_SOURCE_DIR}/lib/libslang.so
    )
elseif(PLATFORM_WINDOWS)
    target_link_directories(slang INTERFACE
        ${slang_SOURCE_DIR}/lib
    )
    target_link_libraries(slang INTERFACE
        gfx
        slang-compiler
        slang-rt
        slang
    )
endif()

#==============================================================================
# Vulkan-related Libraries (already handled by FetchContent)
#==============================================================================
# VMA, volk, and vk-bootstrap are already available from FetchContent
