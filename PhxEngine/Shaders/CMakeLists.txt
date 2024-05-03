
include(../compileshaders.cmake)

file(GLOB PhxEngineShaders
	"*.hlsl"
	"passes/*.hlsl"
	"../include/PhxEngine/shaders/*.hlsli"
	"../include/PhxEngine/shaders/*.h"
)

if(PHX_SHADERS_OUTPUT_DIR)
	set(output_base ${PHX_SHADERS_OUTPUT_DIR})
else()
	set(output_base ${CMAKE_CURRENT_BINARY_DIR})
endif()

phx_compile_shaders_all_platforms(
	TARGET PhxEngineShaders
	CONFIG ${CMAKE_CURRENT_LIST_DIR}/PhxShaders.yml
	FOLDER Phx
	OUTPUT_BASE ${output_base}
	SOURCES ${PhxEngineShaders}
)
