#
# Copyright (c) 2014-2020, NVIDIA CORPORATION. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.


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

    file(MAKE_DIRECTORY ${params_OUTPUT_BASE})

    foreach(FILE ${params_SOURCES})
        get_source_file_property(outputName ${FILE} OutputName)
        get_source_file_property(shadertype ${FILE} ShaderType)
        get_source_file_property(shadermodel ${FILE} ShaderModel)
        get_source_file_property(entryPoint ${FILE} EntryPoint)
        add_custom_command(
            TARGET ${params_TARGET}
            COMMAND ${dxc_SOURCE_DIR}/bin/x64/dxc.exe /nologo /E${entryPoint} /T${shadertype}_6_6 /D "USE_RESOURCE_HEAP" /I "../../PhxEngine/Include" $<IF:$<CONFIG:DEBUG>,/Od,/O3> /Zi /Fo ${OUTPUT_BASE}/dxil/${outputName}.cso /Fd ${OUTPUT_BASE}/dxil/${outputName}.pdb ${FILE}
            MAIN_DEPENDENCY ${FILE}
            COMMENT "HLSL ${FILE}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM)
    endforeach(FILE)

    if(params_FOLDER)
        set_target_properties(${params_TARGET} PROPERTIES FOLDER ${params_FOLDER})
    endif()
endfunction()
