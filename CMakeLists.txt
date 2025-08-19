cmake_minimum_required(VERSION 3.10)
project(glsl-optimizer VERSION 0.1.0)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

include_directories(include)
include_directories(src/mesa)
include_directories(src/mapi)
include_directories(src/glsl)
include_directories(src)

if(WIN32)
	include_directories(src/getopt)
endif()

option (DEBUG "Enable debugging" FALSE)

if(FALSE)
if(${DEBUG} MATCHES "on")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -DNDEBUG")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -DNDEBUG")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")
endif()
endif()

file(GLOB glcpp-library_sources src/glsl/glcpp/*.c src/util/*.c)
# file(GLOB glcpp-library_sources_remove src/glsl/glcpp/glcpp.c)
# list(REMOVE_ITEM glcpp-library_sources ${glcpp-library_sources_remove})
add_library(glcpp-library ${glcpp-library_sources})

file(GLOB mesa_sources src/mesa/program/*.c src/mesa/main/*.c)
add_library(mesa ${mesa_sources})

file(GLOB glsl_sources src/glsl/*.cpp src/glsl/*.c)
file(GLOB glsl_sources_remove src/glsl/main.cpp src/glsl/builtin_stubs.cpp)
list(REMOVE_ITEM glsl_sources ${glsl_sources_remove})

add_library(glsl_optimizer ${glsl_sources})
target_include_directories(glsl_optimizer PUBLIC $<INSTALL_INTERFACE:include>)
target_link_libraries(glsl_optimizer glcpp-library mesa)

# Single lib combining all three of the above (for linux to workaround linking
# problems but could become default for all).
add_library(glsl_optimizer_combined ${glcpp-library_sources} ${mesa_sources} ${glsl_sources})
target_include_directories(glsl_optimizer_combined PUBLIC $<INSTALL_INTERFACE:include>)

if(WIN32)
	add_executable(glsl_compiler src/glsl/main.cpp src/getopt/getopt_long.c)
else()
	add_executable(glsl_compiler src/glsl/main.cpp)
endif()
target_link_libraries(glsl_compiler PRIVATE glsl_optimizer)

file(GLOB glsl_test_sources tests/*.cpp)
add_executable(glsl_test ${glsl_test_sources})
target_link_libraries(glsl_test PRIVATE glsl_optimizer)
target_link_libraries(glsl_test PRIVATE OpenGL::GL)

file(GLOB glslopt_sources contrib/glslopt/*.cpp)
add_executable(glslopt ${glslopt_sources})
target_link_libraries(glslopt PRIVATE glsl_optimizer)

# Create an alias for the target that vcpkg expects
# add_library(glsl-optimizer::glsl_optimizer_combined ALIAS glsl_optimizer_combined)

#--------------------------------------------------------------------
# Install
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(INSTALL_TARGETS "mesa;glcpp-library;glsl_optimizer;glsl_optimizer_combined")

install(TARGETS ${INSTALL_TARGETS}
	EXPORT ${PROJECT_NAME}Targets)

install(TARGETS ${INSTALL_TARGETS}
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(TARGETS glsl_compiler glslopt
	DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(EXPORT ${PROJECT_NAME}Targets
	FILE ${PROJECT_NAME}Targets.cmake
	NAMESPACE ${PROJECT_NAME}::
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
	INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
#	NO_SET_AND_CHECK_MACRO
#	NO_CHECK_REQUIRED_COMPONENTS_MACRO)

write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	VERSION "${PROJECT_VERSION}"
	COMPATIBILITY AnyNewerVersion)

install(FILES src/glsl/glsl_optimizer.h
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")

install(FILES license.txt
	DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
	RENAME copyright)

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
