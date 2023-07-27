# generates a build target that will compile shaders for a given config file
#
# usage: donut_compile_shaders(TARGET <generated build target name>
#                              CONFIG <shader-config-file>
#                              [DXIL <dxil-output-path>]
#                              [DXBC <dxbc-output-path>]
#                              [SPIRV_DXC <spirv-output-path>])

function(dxc_compile_shaders)
    set(options "")
    set(oneValueArgs TARGET CONFIG FOLDER OUTPUT_BASE CFLAGS)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(params "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})


    # just add the source files to the project as documents, they are built by the script
    set_source_files_properties(${params_SOURCES} PROPERTIES VS_TOOL_OVERRIDE "None") 
    add_custom_target(${params_TARGET}
        DEPENDS shaderCompiler
        SOURCES ${params_SOURCES})

    if (NOT params_TARGET)
        message(FATAL_ERROR "dxc_compile_shaders: TARGET argument missing")
    endif()
    if (NOT params_CONFIG)
        message(FATAL_ERROR "dxc_compile_shaders: CONFIG argument missing")
    endif()
    if (NOT params_OUTPUT_BASE)
        message(FATAL_ERROR "dxc_compile_shaders: OUTPUT_BASE argument missing")
    endif()

    message("--->Includes: ${INCLUDES}")

    set(OUTPUT_DIR ${params_OUTPUT_BASE}/dxil)
    file(MAKE_DIRECTORY ${OUTPUT_DIR})
    message(${OUTPUT_DIR})

    foreach(FILE ${params_SOURCES})
        get_filename_component(FILE_WE ${FILE} NAME_WE)
        get_source_file_property(shadertype ${FILE} ShaderType)
        get_source_file_property(shadermodel ${FILE} ShaderModel)
        get_source_file_property(entryPoint ${FILE} EntryPoint)

        add_custom_command(
            TARGET ${params_TARGET}
            COMMAND ${dxc_SOURCE_DIR}/bin/x64/dxc.exe /nologo /E${entryPoint} /T${shadertype}_6_6 /D "USE_RESOURCE_HEAP" $<IF:$<CONFIG:DEBUG>,/Od,/O3> /Zi /Fo ${OUTPUT_DIR}/${FILE_WE}.cso /Fd ${OUTPUT_DIR}/${FILE_WE}.pdb ${FILE}
            MAIN_DEPENDENCY ${FILE}
            COMMENT "HLSL ${FILE}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM)
    endforeach(FILE)

    if(params_FOLDER)
        set_target_properties(${params_TARGET} PROPERTIES FOLDER ${params_FOLDER})
    endif()
endfunction()
