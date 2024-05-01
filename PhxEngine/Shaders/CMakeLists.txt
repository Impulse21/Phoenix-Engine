
include(../compileshaders.cmake)

file(GLOB phx_shaders
	"*.hlsl"
	"passes/*.hlsl"
	"../include/PhxEngine/shaders/*.hlsli"
	"../include/PhxEngine/shaders/*.h"
)

if(PHX_SHADERS_OUTPUT_DIR)
	set(output_base ${DONUT_SHADERS_OUTPUT_DIR})
else()
	set(output_base ${CMAKE_CURRENT_BINARY_DIR})
endif()

phx_compile_shaders_all_platforms(
	TARGET donut_shaders
	CONFIG ${CMAKE_CURRENT_LIST_DIR}/PhxShaders.yml
	FOLDER Phx
	OUTPUT_BASE ${output_base}
	SOURCES ${donut_shaders}
)
