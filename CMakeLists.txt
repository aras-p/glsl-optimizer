cmake_minimum_required(VERSION 3.21)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not supported. Use: cmake -S . -B build")
endif()

project(glsl_optimizer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# use static MSVC runtime
if(MSVC AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(GLSLOPT_BUILD_TOOLS "Build glslopt command-line tool" OFF)
option(GLSLOPT_TEST_OPENGL "Build OpenGL validation into the test runner" OFF)

add_library(glsl_optimizer STATIC
    src/compiler/glsl/ast_array_index.cpp
    src/compiler/glsl/ast_expr.cpp
    src/compiler/glsl/ast_function.cpp
    src/compiler/glsl/ast_to_hir.cpp
    src/compiler/glsl/ast_type.cpp
    src/compiler/glsl/builtin_functions.cpp
    src/compiler/glsl/builtin_types.cpp
    src/compiler/glsl/builtin_variables.cpp
    src/compiler/glsl/generate_ir.cpp
    src/compiler/glsl/glcpp/pp.c
    src/compiler/glsl/glsl_parser_extras.cpp
    src/compiler/glsl/glsl_symbol_table.cpp
    src/compiler/glsl/hir_field_selection.cpp
    src/compiler/glsl/ir_precision.cpp
    src/compiler/glsl/ir.cpp
    src/compiler/glsl/ir_array_refcount.cpp
    src/compiler/glsl/ir_basic_block.cpp
    src/compiler/glsl/ir_builder.cpp
    src/compiler/glsl/ir_clone.cpp
    src/compiler/glsl/ir_constant_expression.cpp
    src/compiler/glsl/ir_equals.cpp
    src/compiler/glsl/ir_expression_flattening.cpp
    src/compiler/glsl/ir_function.cpp
    src/compiler/glsl/ir_function_can_inline.cpp
    src/compiler/glsl/ir_function_detect_recursion.cpp
    src/compiler/glsl/ir_hierarchical_visitor.cpp
    src/compiler/glsl/ir_hv_accept.cpp
    src/compiler/glsl/ir_print_visitor.cpp
    src/compiler/glsl/ir_rvalue_visitor.cpp
    src/compiler/glsl/ir_set_program_inouts.cpp
    src/compiler/glsl/ir_validate.cpp
    src/compiler/glsl/ir_variable_refcount.cpp
    src/compiler/glsl/link_atomics.cpp
    src/compiler/glsl/link_functions.cpp
    src/compiler/glsl/link_interface_blocks.cpp
    src/compiler/glsl/link_uniform_block_active_visitor.cpp
    src/compiler/glsl/link_uniform_blocks.cpp
    src/compiler/glsl/link_uniform_initializers.cpp
    src/compiler/glsl/link_uniforms.cpp
    src/compiler/glsl/link_varyings.cpp
    src/compiler/glsl/linker.cpp
    src/compiler/glsl/linker_util.cpp
    src/compiler/glsl/loop_analysis.cpp
    src/compiler/glsl/loop_unroll.cpp
    src/compiler/glsl/lower_buffer_access.cpp
    src/compiler/glsl/lower_builtins.cpp
    src/compiler/glsl/lower_const_arrays_to_uniforms.cpp
    src/compiler/glsl/lower_cs_derived.cpp
    src/compiler/glsl/lower_discard_flow.cpp
    src/compiler/glsl/lower_distance.cpp
    src/compiler/glsl/lower_instructions.cpp
    src/compiler/glsl/lower_jumps.cpp
    src/compiler/glsl/lower_named_interface_blocks.cpp
    src/compiler/glsl/lower_packed_varyings.cpp
    src/compiler/glsl/lower_shared_reference.cpp
    src/compiler/glsl/lower_subroutine.cpp
    src/compiler/glsl/lower_tess_level.cpp
    src/compiler/glsl/lower_ubo_reference.cpp
    src/compiler/glsl/lower_vec_index_to_swizzle.cpp
    src/compiler/glsl/lower_vector_derefs.cpp
    src/compiler/glsl/lower_vector_insert.cpp
    src/compiler/glsl/lower_vertex_id.cpp
    src/compiler/glsl/lower_xfb_varying.cpp
    src/compiler/glsl/opt_algebraic.cpp
    src/compiler/glsl/opt_array_splitting.cpp
    src/compiler/glsl/opt_conditional_discard.cpp
    src/compiler/glsl/opt_constant_folding.cpp
    src/compiler/glsl/opt_constant_propagation.cpp
    src/compiler/glsl/opt_constant_variable.cpp
    src/compiler/glsl/opt_copy_propagation_elements.cpp
    src/compiler/glsl/opt_dead_builtin_variables.cpp
    src/compiler/glsl/opt_dead_builtin_varyings.cpp
    src/compiler/glsl/opt_dead_code.cpp
    src/compiler/glsl/opt_dead_code_local.cpp
    src/compiler/glsl/opt_dead_functions.cpp
    src/compiler/glsl/opt_flatten_nested_if_blocks.cpp
    src/compiler/glsl/opt_flip_matrices.cpp
    src/compiler/glsl/opt_function_inlining.cpp
    src/compiler/glsl/opt_if_simplification.cpp
    src/compiler/glsl/opt_minmax.cpp
    src/compiler/glsl/opt_rebalance_tree.cpp
    src/compiler/glsl/opt_redundant_jumps.cpp
    src/compiler/glsl/opt_structure_splitting.cpp
    src/compiler/glsl/opt_swizzle.cpp
    src/compiler/glsl/opt_tree_grafting.cpp
    src/compiler/glsl/opt_vectorize.cpp
    src/compiler/glsl/propagate_invariance.cpp
    src/compiler/glsl/standalone_scaffolding.cpp
    src/compiler/glsl/string_to_uint_map.cpp
    src/compiler/glsl_types.cpp
    src/compiler/shader_enums.c
    src/mesa/program/symbol_table.c
    src/util/bitscan.c
    src/util/debug.c
    src/util/half_float.c
    src/util/hash_table.c
    src/util/os_misc.c
    src/util/ralloc.c
    src/util/set.c
    src/util/string_buffer.c
    src/util/strtod.c
    src/util/u_debug.c
    src/compiler/glsl/glsl_parser.cpp
    src/compiler/glsl/glsl_lexer.cpp
    src/compiler/glsl/glcpp/glcpp-parse.c
    src/compiler/glsl/glcpp/glcpp-lex.c
    src/glsl/glsl_optimizer.cpp
    src/glsl/ir_stats.cpp
    src/glsl/ir_print_glsl_visitor.cpp
    src/glsl/ir_print_metal_visitor.cpp
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
    set_target_properties(glsl_optimizer PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_compile_features(glsl_optimizer PRIVATE c_std_99)
find_package(Threads REQUIRED)
target_link_libraries(glsl_optimizer PRIVATE Threads::Threads)
target_include_directories(glsl_optimizer
    PUBLIC src/glsl
    PRIVATE include src src/mesa src/compiler src/compiler/glsl
        src/compiler/glsl/glcpp src/gallium/include
)
# generated Flex scanner uses 'register', which clang rejects
set_source_files_properties(src/compiler/glsl/glsl_lexer.cpp PROPERTIES
    COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-register>"
)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(glsl_optimizer PRIVATE
        HAVE___BUILTIN_EXPECT HAVE___BUILTIN_CLZ HAVE___BUILTIN_CLZLL
        HAVE___BUILTIN_CTZ HAVE___BUILTIN_FFS HAVE___BUILTIN_FFSLL
        HAVE___BUILTIN_POPCOUNT HAVE___BUILTIN_POPCOUNTLL
    )
endif()
include(CheckSymbolExists)
check_symbol_exists(posix_memalign "stdlib.h" GLSLOPT_HAVE_POSIX_MEMALIGN)
check_symbol_exists(strtof "stdlib.h" GLSLOPT_HAVE_STRTOF)
if(GLSLOPT_HAVE_POSIX_MEMALIGN)
    target_compile_definitions(glsl_optimizer PRIVATE HAVE_POSIX_MEMALIGN)
endif()
if(GLSLOPT_HAVE_STRTOF)
    target_compile_definitions(glsl_optimizer PRIVATE HAVE_STRTOF)
endif()
if(MSVC)
    target_compile_definitions(glsl_optimizer PRIVATE
        NOMINMAX _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE strdup=_strdup
        # the checked-in flex scanner requires this for const-correct prototypes
        __STDC__=1
    )
endif()

if(GLSLOPT_BUILD_TOOLS)
    add_executable(glslopt contrib/glslopt/Main.cpp)
    target_link_libraries(glslopt PRIVATE glsl_optimizer)
    if(MSVC)
        target_compile_definitions(glslopt PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
endif()

# do not build tests by default when embedded with add_subdirectory
set(GLSLOPT_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(GLSLOPT_TOP_LEVEL ON)
endif()
option(BUILD_TESTING "Build optimizer tests" ${GLSLOPT_TOP_LEVEL})
include(CTest)
if(BUILD_TESTING)
    add_executable(glsl_optimizer_tests tests/glsl_optimizer_tests.cpp)
    target_link_libraries(glsl_optimizer_tests PRIVATE glsl_optimizer)
    if(MSVC)
        target_compile_definitions(glsl_optimizer_tests PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
    endif()
    if(GLSLOPT_TEST_OPENGL)
        if(NOT WIN32 AND NOT APPLE)
            message(FATAL_ERROR "Optional OpenGL validation is supported only on Windows and macOS")
        endif()
        find_package(OpenGL REQUIRED)
        target_link_libraries(glsl_optimizer_tests PRIVATE OpenGL::GL)
        target_compile_definitions(glsl_optimizer_tests PRIVATE GOT_GFX=1)
    endif()
    add_test(NAME shaders COMMAND glsl_optimizer_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    set_tests_properties(shaders PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    add_test(NAME test_runner COMMAND ${CMAKE_COMMAND}
        "-DTEST_RUNNER=$<TARGET_FILE:glsl_optimizer_tests>"
        "-DTEST_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_runner.cmake"
    )
endif()
