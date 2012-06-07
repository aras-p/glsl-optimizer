cmake_minimum_required(VERSION 2.8)

include_directories(../include)
include_directories(../src/mesa)
include_directories(../src/glsl)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -O4 -DNDEBUG -DUNIX")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -O4 -DNDEBUG -DUNIX")

file(GLOB agalasm_sources ../agalassembler/*.cpp)
file(GLOB glcpp-library_sources ../src/glsl/glcpp/*.c)
file(GLOB glcpp-library_sources_remove ../src/glsl/glcpp/glcpp.c)
list(REMOVE_ITEM glcpp-library_sources ${glcpp-library_sources_remove})
file(GLOB mesa_sources ../src/mesa/program/*.c)
file(GLOB glsl_sources ../src/glsl/*.cpp ../src/glsl/*.c)
file(GLOB glsl_sources_remove ../src/glsl/main.cpp ../src/glsl/builtin_stubs.cpp ../src/glsl/swc.cpp)
list(REMOVE_ITEM glsl_sources ${glsl_sources_remove})

add_library(glsl2agal ${agalasm_sources} ${glcpp-library_sources} ${mesa_sources} ${glsl_sources})

add_custom_command(TARGET glsl2agal POST_BUILD COMMAND g++ -O4 -flto-api=exports.txt -falcon -emit-swc -o glsl2agal.swc agaloptimiser.abc swc.cpp libglsl2agal.a -I../include -I../src/mesa -I../src/mapi -I../src/glsl)

add_custom_command(TARGET glsl2agal POST_BUILD COMMAND g++ -DCMDLINE=1 -O4 -falcon -flto-api=exports.txt -o glsl2agalcompiler agaloptimiser.abc swc.cpp libglsl2agal.a -I../include -I../src/mesa -I../src/mapi -I../src/glsl)

add_custom_command(TARGET glsl2agal PRE_BUILD COMMAND cd ../agaloptimiser/src && bash genabc.sh  && cd ../../swc && cp ../agaloptimiser/src/foo.abc agaloptimiser.abc)