
# Required dependencies

add_executable(TestBed Src/TestBedApp.cpp)
target_link_libraries(TestBed PhxEngineApp PhxEngineRHI_Dx12)

set_property(TARGET  PROPERTY FOLDER "Applications")