
# generates a build target that will compile shaders for a given config file
#
# usage: phx_compile_shaders(TARGET <generated build target name>
#                              CONFIG <shader-config-file>
#                              [DXIL <dxil-output-path>]
#                              [SPIRV_DXC <spirv-output-path>])

function(phx_compile_shaders)
    set(options "")
    set(oneValueArgs TARGET CONFIG FOLDER DXIL SPIRV_DXC CFLAGS)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(params "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT params_TARGET)
        message(FATAL_ERROR "phx_compile_shaders: TARGET argument missing")
    endif()
    if (NOT params_CONFIG)
        message(FATAL_ERROR "phx_compile_shaders: CONFIG argument missing")
    endif()

    # just add the source files to the project as documents, they are built by the script
    set_source_files_properties(${params_SOURCES} PROPERTIES VS_TOOL_OVERRIDE "None") 

    add_custom_target(${params_TARGET}
        DEPENDS PhxShaderCompiler
        SOURCES ${params_SOURCES})

    if (params_DXIL AND PHX_WITH_DX12)
        #if (NOT params_CFLAGS)
        #    set(CFLAGS "$<IF:$<CONFIG:Debug>,-Zi -Qembed_debug,-Qstrip_debug -Qstrip_reflect> -O3 -WX -Zpr")
        #else()
        #    set(CFLAGS ${params_CFLAGS})
        #endif()

        add_custom_command(TARGET ${params_TARGET} PRE_BUILD
                          COMMAND PhxShaderCompiler
                                   --infile ${params_CONFIG}
                                   --parallel
                                   --out ${params_DXIL}
                                   --platform dxil
                                   --cflags "${CFLAGS}"
                                   -I ${DONUT_SHADER_INCLUDE_DIR}
                                   --compiler ${DXC_DXIL_EXECUTABLE})
    endif()

    if (params_SPIRV_DXC AND PHX_WITH_VULKAN)
            message(FATAL_ERROR "phx_compile_shaders: DXC for SPIR-V Not Supported yet")
    endif()

    if(params_FOLDER)
        set_target_properties(${params_TARGET} PROPERTIES FOLDER ${params_FOLDER})
    endif()
endfunction()

# Generates a build target that will compile shaders for a given config file for all enabled Phx platforms.
#
# The shaders will be placed into subdirectories of ${OUTPUT_BASE}, with names compatible with
# the FindDirectoryWithShaderBin framework function.

function(phx_compile_shaders_all_platforms)
    set(options "")
    set(oneValueArgs TARGET CONFIG FOLDER OUTPUT_BASE CFLAGS)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(params "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT params_TARGET)
        message(FATAL_ERROR "phx_compile_shaders_all_platforms: TARGET argument missing")
    endif()
    if (NOT params_CONFIG)
        message(FATAL_ERROR "phx_compile_shaders_all_platforms: CONFIG argument missing")
    endif()
    if (NOT params_OUTPUT_BASE)
        message(FATAL_ERROR "phx_compile_shaders_all_platforms: OUTPUT_BASE argument missing")
    endif()

    phx_compile_shaders(TARGET ${params_TARGET}
                          CONFIG ${params_CONFIG}
                          FOLDER ${params_FOLDER}
                          CFLAGS ${params_CFLAGS}
                          DXIL ${params_OUTPUT_BASE}/dxil
                          SPIRV_DXC ${params_OUTPUT_BASE}/spirv
                          SOURCES ${params_SOURCES})

endfunction()
