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
    src/glsl/glsl_optimizer.cpp
    src/glsl/nir_optimize.cpp
    src/glsl/nir_printer.cpp
    src/glsl/nir_print_glsl.cpp
    src/glsl/nir_print_metal.cpp
    src/c11/threads.cpp
    src/compiler/builtin_types.c
    src/compiler/glsl/ast_array_index.cpp
    src/compiler/glsl/ast_expr.cpp
    src/compiler/glsl/ast_function.cpp
    src/compiler/glsl/ast_to_hir.cpp
    src/compiler/glsl/ast_type.cpp
    src/compiler/glsl/builtin_functions.cpp
    src/compiler/glsl/builtin_types.cpp
    src/compiler/glsl/builtin_variables.cpp
    src/compiler/glsl/glcpp/glcpp-lex.c
    src/compiler/glsl/glcpp/glcpp-parse.c
    src/compiler/glsl/glcpp/pp.c
    src/compiler/glsl/glcpp/pp_standalone_scaffolding.c
    src/compiler/glsl/glsl_lexer.cpp
    src/compiler/glsl/glsl_parser.cpp
    src/compiler/glsl/glsl_parser_extras.cpp
    src/compiler/glsl/glsl_symbol_table.cpp
    src/compiler/glsl/glsl_to_nir.cpp
    src/compiler/glsl/hir_field_selection.cpp
    src/compiler/glsl/ir.cpp
    src/compiler/glsl/ir_builder.cpp
    src/compiler/glsl/ir_clone.cpp
    src/compiler/glsl/ir_constant_expression.cpp
    src/compiler/glsl/ir_expression_flattening.cpp
    src/compiler/glsl/ir_function.cpp
    src/compiler/glsl/ir_function_detect_recursion.cpp
    src/compiler/glsl/ir_hierarchical_visitor.cpp
    src/compiler/glsl/ir_hv_accept.cpp
    src/compiler/glsl/ir_print_visitor.cpp
    src/compiler/glsl/ir_rvalue_visitor.cpp
    src/compiler/glsl/ir_validate.cpp
    src/compiler/glsl/linker_util.cpp
    src/compiler/glsl/lower_builtins.cpp
    src/compiler/glsl/lower_instructions.cpp
    src/compiler/glsl/lower_mat_op_to_vec.cpp
    src/compiler/glsl/lower_packing_builtins.cpp
    src/compiler/glsl/lower_subroutine.cpp
    src/compiler/glsl/lower_vec_index_to_cond_assign.cpp
    src/compiler/glsl/lower_vector_derefs.cpp
    src/compiler/glsl/opt_dead_builtin_variables.cpp
    src/compiler/glsl/opt_function_inlining.cpp
    src/compiler/glsl/standalone_scaffolding.cpp
    src/compiler/glsl_types.c
    src/compiler/nir/nir.c
    src/compiler/nir/nir_builder.c
    src/compiler/nir/nir_builtin_builder.c
    src/compiler/nir/nir_clone.c
    src/compiler/nir/nir_constant_expressions.c
    src/compiler/nir/nir_control_flow.c
    src/compiler/nir/nir_deref.c
    src/compiler/nir/nir_dominance.c
    src/compiler/nir/nir_dominance_lca.c
    src/compiler/nir/nir_from_ssa.c
    src/compiler/nir/nir_functions.c
    src/compiler/nir/nir_gather_types.c
    src/compiler/nir/nir_glslopt_helpers.c
    src/compiler/nir/nir_instr_set.c
    src/compiler/nir/nir_intrinsics.c
    src/compiler/nir/nir_liveness.c
    src/compiler/nir/nir_loop_analyze.c
    src/compiler/nir/nir_lower_continue_constructs.c
    src/compiler/nir/nir_lower_global_vars_to_local.c
    src/compiler/nir/nir_lower_reg_intrinsics_to_ssa.c
    src/compiler/nir/nir_lower_returns.c
    src/compiler/nir/nir_lower_system_values.c
    src/compiler/nir/nir_lower_var_copies.c
    src/compiler/nir/nir_lower_variable_initializers.c
    src/compiler/nir/nir_lower_vars_to_ssa.c
    src/compiler/nir/nir_metadata.c
    src/compiler/nir/nir_opcodes.c
    src/compiler/nir/nir_opt_algebraic.c
    src/compiler/nir/nir_opt_combine_stores.c
    src/compiler/nir/nir_opt_constant_folding.c
    src/compiler/nir/nir_opt_copy_prop_vars.c
    src/compiler/nir/nir_opt_copy_propagate.c
    src/compiler/nir/nir_opt_cse.c
    src/compiler/nir/nir_opt_dce.c
    src/compiler/nir/nir_opt_dead_cf.c
    src/compiler/nir/nir_opt_dead_write_vars.c
    src/compiler/nir/nir_opt_gcm.c
    src/compiler/nir/nir_opt_if.c
    src/compiler/nir/nir_opt_loop_unroll.c
    src/compiler/nir/nir_opt_move.c
    src/compiler/nir/nir_opt_peephole_select.c
    src/compiler/nir/nir_opt_remove_phis.c
    src/compiler/nir/nir_opt_shrink_stores.c
    src/compiler/nir/nir_opt_sink.c
    src/compiler/nir/nir_opt_undef.c
    src/compiler/nir/nir_phi_builder.c
    src/compiler/nir/nir_print.c
    src/compiler/nir/nir_range_analysis.c
    src/compiler/nir/nir_remove_dead_variables.c
    src/compiler/nir/nir_repair_ssa.c
    src/compiler/nir/nir_search.c
    src/compiler/nir/nir_split_var_copies.c
    src/compiler/nir/nir_split_vars.c
    src/compiler/nir/nir_to_lcssa.c
    src/compiler/nir/nir_validate.c
    src/compiler/shader_enums.c
    src/mesa/main/extensions_table.c
    src/mesa/program/dummy_errors.c
    src/mesa/program/symbol_table.c
    src/util/blob.c
    src/util/double.c
    src/util/float8.c
    src/util/format/u_format_table.c
    src/util/half_float.c
    src/util/hash_table.c
    src/util/log.c
    src/util/memstream.c
    src/util/os_misc.c
    src/util/ralloc.c
    src/util/range_minimum_query.c
    src/util/rb_tree.c
    src/util/set.c
    src/util/softfloat.c
    src/util/string_buffer.c
    src/util/strndup.c
    src/util/strtod.c
    src/util/u_debug.c
    src/util/u_dynarray.c
    src/util/u_printf.c
    src/util/u_qsort.cpp
    src/util/u_range_remap.c
    src/util/u_vector.c
    src/util/u_worklist.c
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
    set_target_properties(glsl_optimizer PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_compile_features(glsl_optimizer PRIVATE c_std_11)
find_package(Threads REQUIRED)
target_link_libraries(glsl_optimizer PRIVATE Threads::Threads)
target_include_directories(glsl_optimizer
    PUBLIC src/glsl
    PRIVATE include src src/mesa src/gallium/include src/compiler src/compiler/glsl
        src/compiler/glsl/glcpp
)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_definitions(glsl_optimizer PRIVATE
        HAVE___BUILTIN_EXPECT HAVE___BUILTIN_CLZ HAVE___BUILTIN_CLZLL
        HAVE___BUILTIN_CTZ HAVE___BUILTIN_FFS HAVE___BUILTIN_FFSLL
        HAVE___BUILTIN_POPCOUNT HAVE___BUILTIN_POPCOUNTLL
    )
endif()
include(CheckSymbolExists)
include(CheckIncludeFile)
check_include_file(endian.h GLSLOPT_HAVE_ENDIAN_H)
if(GLSLOPT_HAVE_ENDIAN_H)
    target_compile_definitions(glsl_optimizer PRIVATE HAVE_ENDIAN_H)
endif()
check_symbol_exists(posix_memalign "stdlib.h" GLSLOPT_HAVE_POSIX_MEMALIGN)
check_symbol_exists(strtof "stdlib.h" GLSLOPT_HAVE_STRTOF)
if(GLSLOPT_HAVE_POSIX_MEMALIGN)
    target_compile_definitions(glsl_optimizer PRIVATE HAVE_POSIX_MEMALIGN)
endif()
if(GLSLOPT_HAVE_STRTOF)
    target_compile_definitions(glsl_optimizer PRIVATE HAVE_STRTOF)
endif()
# GL APIs the frontend accepts (desktop GL and GLES2+; checked by main/context.h)
target_compile_definitions(glsl_optimizer PRIVATE HAVE_OPENGL=1 HAVE_OPENGL_ES_2=1)
# GLSL IR's operator new stores into the object before its constructor runs;
# GCC's lifetime dead-store elimination would drop that (upstream Mesa uses this too)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(glsl_optimizer PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-flifetime-dse=1>)
endif()
if(MSVC)
    # warnings upstream Mesa disables for MSVC (meson.build)
    target_compile_options(glsl_optimizer PRIVATE
        /wd4018 /wd4056 /wd4244 /wd4267 /wd4305 /wd4351 /wd4756 /wd4800 /wd4291 /wd4146
        /wd4200 /wd4624 /wd4309 /wd4838
    )
    target_compile_definitions(glsl_optimizer PRIVATE
        NOMINMAX _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE strdup=_strdup
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
if(CMAKE_JS_VERSION)
    set(GLSLOPT_TOP_LEVEL OFF) # building the Node addon
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

# Node addon, when built through cmake-js (npm install)
if(CMAKE_JS_VERSION)
    execute_process(COMMAND node -e "require('nan')" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE NAN_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    add_library(glslOptimizer SHARED src/node/binding.cpp src/node/compiler.cpp src/node/shader.cpp ${CMAKE_JS_SRC})
    set_target_properties(glslOptimizer PROPERTIES PREFIX "" SUFFIX ".node")
    target_include_directories(glslOptimizer PRIVATE ${CMAKE_JS_INC} ${NAN_INCLUDE_DIR})
    target_link_libraries(glslOptimizer PRIVATE glsl_optimizer ${CMAKE_JS_LIB})
    set_target_properties(glsl_optimizer PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
        # node.lib import library for the addon to link against
        execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
    endif()
endif()
