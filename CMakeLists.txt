cmake_minimum_required(VERSION 2.8)

include_directories(include)
include_directories(src/mesa)
include_directories(src/mapi)
include_directories(src/glsl)

#option (DEBUG "Enable debugging" "on")

#if(${DEBUG} MATCHES "on")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DUNIX")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -DUNIX")
#else()
#   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -DNDEBUG -DUNIX")
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -DNDEBUG -DUNIX")
#    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")
#endif()

file(GLOB agalasm_sources agalassembler/*.cpp)
add_library(agalasm ${agalasm_sources})

file(GLOB glcpp-library_sources src/glsl/glcpp/*.c)
file(GLOB glcpp-library_sources_remove src/glsl/glcpp/glcpp.c)
list(REMOVE_ITEM glcpp-library_sources ${glcpp-library_sources_remove})
add_library(glcpp-library ${glcpp-library_sources})

file(GLOB mesa_sources src/mesa/program/*.c)
add_library(mesa ${mesa_sources})

file(GLOB glsl_sources src/glsl/*.cpp src/glsl/*.c)
file(GLOB glsl_sources_remove src/glsl/main.cpp src/glsl/builtin_stubs.cpp)
list(REMOVE_ITEM glsl_sources ${glsl_sources_remove})
add_library(glsl_optimizer ${glsl_sources})
target_link_libraries(glsl_optimizer glcpp-library mesa agalasm)

add_executable(glsl_compiler src/glsl/main.cpp)
target_link_libraries(glsl_compiler glsl_optimizer agalasm)

file(GLOB glsl_test_sources tests/*.cpp)
add_executable(glsl_test ${glsl_test_sources})
target_link_libraries(glsl_test glsl_optimizer agalasm)

file(GLOB glslopt_sources contrib/glslopt/*.cpp)
add_executable(glslopt ${glslopt_sources})
target_link_libraries(glslopt glsl_optimizer agalasm)

add_executable(glcpp src/glsl/glcpp/glcpp.c)
target_link_libraries(glcpp glsl_optimizer agalasm)