cmake_minimum_required(VERSION 2.8)

include_directories(include)
include_directories(src/mesa)
include_directories(src/mapi)

file(GLOB glcpp_sources src/glsl/glcpp/*.c)
add_library(glcpp ${glcpp_sources})

file(GLOB mesa_sources src/mesa/program/*.c)
add_library(mesa ${mesa_sources})

file(GLOB glsl_sources src/glsl/*.cpp src/glsl/*.c)
list(REMOVE_ITEM glsl_sources src/glsl/main.cpp)
add_library(glsl_optimizer ${glsl_sources})
target_link_libraries(glsl_optimizer mesa glcpp)

file(GLOB glsl_test_sources tests/*.cpp)
add_executable(glsl_test ${glsl_test_sources})
target_link_libraries(glsl_test glsl_optimizer)
