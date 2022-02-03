
# Required dependencies

file(GLOB TestBed_Src
    Include/*.h
    Src/*.h
    Src/*.cpp)

add_executable(TestBed ${TestBed_Src})
target_link_libraries(TestBed PhxEngine PhxEngineShaders)

target_include_directories(TestBed PRIVATE ../Shaders/Include)


set_property(TARGET  PROPERTY FOLDER "Applications")