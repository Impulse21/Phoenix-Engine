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

target_include_directories(ImGui SYSTEM PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(ImGui PUBLIC
    glfw
    Vulkan::Vulkan
)

# Visual Studio organization
set_target_properties(ImGui PROPERTIES FOLDER "_Dependencies")
if(COMMAND apply_filters)
    apply_filters(ImGui)
endif()

optimize_vendor_target(ImGui)
target_compile_options(ImGui PRIVATE -w)

#==============================================================================
# TLSF Allocator
#==============================================================================
add_library(tlsf STATIC
    ${tlsf_SOURCE_DIR}/tlsf.c
)

target_include_directories(tlsf SYSTEM PUBLIC
    ${tlsf_SOURCE_DIR}
)

# Visual Studio organization
set_target_properties(tlsf PROPERTIES FOLDER "_Dependencies")

# Apply source filters for Visual Studio
if(COMMAND apply_filters)
    apply_filters(tlsf)
endif()

optimize_vendor_target(tlsf)
target_compile_options(tlsf PRIVATE -w)

#==============================================================================
# bc7enc_rdo
#==============================================================================
add_library(bc7enc_rdo STATIC
    ${bc7enc_rdo_SOURCE_DIR}/bc7enc.cpp
    ${bc7enc_rdo_SOURCE_DIR}/rgbcx.cpp
)

target_include_directories(bc7enc_rdo SYSTEM PUBLIC
    ${bc7enc_rdo_SOURCE_DIR}
)

# Visual Studio organization
set_target_properties(bc7enc_rdo PROPERTIES FOLDER "_Dependencies")

# Apply source filters for Visual Studio
if(COMMAND apply_filters)
    apply_filters(bc7enc_rdo)
endif()

optimize_vendor_target(bc7enc_rdo)
target_compile_options(bc7enc_rdo PRIVATE -w)

#==============================================================================
# Header-Only Libraries (Interface targets)
#==============================================================================
# hlslpp
add_library(hlslpp INTERFACE)
target_include_directories(hlslpp SYSTEM INTERFACE
    ${hlslpp_SOURCE_DIR}/include
)

# stb
add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE
    ${stb_SOURCE_DIR}
)

# cgltf
add_library(cgltf INTERFACE)
target_include_directories(cgltf SYSTEM INTERFACE
    ${cgltf_SOURCE_DIR}
)

# Slang Shader Compiler (pre-built binaries)
add_library(slang INTERFACE)
target_include_directories(slang SYSTEM INTERFACE
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

set(VENDOR_TARGETS
    yaml-cpp
    reflectcpp
    glfw
    meshoptimizer
    volk
    vk-bootstrap
    TracyClient
)

# 4. Loop and Optimize
foreach(LIB ${VENDOR_TARGETS})
    set_target_properties(${LIB} PROPERTIES FOLDER "_Dependencies")
endforeach()

set_target_properties(spdlog PROPERTIES FOLDER "_Dependencies/Unused")
if(TARGET update_mappings)
    set_target_properties(update_mappings PROPERTIES FOLDER "_Dependencies/Utils")
endif()