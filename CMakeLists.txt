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

set(SRC
    # C sources
    "src/glsl/glcpp/glcpp-lex.c"
    "src/glsl/glcpp/glcpp-parse.c"
    "src/glsl/glcpp/pp.c"
    "src/glsl/strtod.c"
    "src/mesa/main/imports.c"
    "src/mesa/program/prog_hash_table.c"
    "src/mesa/program/symbol_table.c"
    "src/util/hash_table.c"
    "src/util/ralloc.c"

    # C++ sources
    "src/glsl/ast_array_index.cpp"
    "src/glsl/ast_expr.cpp"
    "src/glsl/ast_function.cpp"
    "src/glsl/ast_to_hir.cpp"
    "src/glsl/ast_type.cpp"
    "src/glsl/builtin_functions.cpp"
    "src/glsl/builtin_types.cpp"
    "src/glsl/builtin_variables.cpp"
    "src/glsl/glsl_lexer.cpp"
    "src/glsl/glsl_optimizer.cpp"
    "src/glsl/glsl_parser.cpp"
    "src/glsl/glsl_parser_extras.cpp"
    "src/glsl/glsl_symbol_table.cpp"
    "src/glsl/glsl_types.cpp"
    "src/glsl/hir_field_selection.cpp"
    "src/glsl/ir.cpp"
    "src/glsl/ir_basic_block.cpp"
    "src/glsl/ir_builder.cpp"
    "src/glsl/ir_clone.cpp"
    "src/glsl/ir_constant_expression.cpp"
    "src/glsl/ir_equals.cpp"
    "src/glsl/ir_expression_flattening.cpp"
    "src/glsl/ir_function.cpp"
    "src/glsl/ir_function_can_inline.cpp"
    "src/glsl/ir_function_detect_recursion.cpp"
    "src/glsl/ir_hierarchical_visitor.cpp"
    "src/glsl/ir_hv_accept.cpp"
    "src/glsl/ir_import_prototypes.cpp"
    "src/glsl/ir_print_glsl_visitor.cpp"
    "src/glsl/ir_print_metal_visitor.cpp"
    "src/glsl/ir_print_visitor.cpp"
    "src/glsl/ir_rvalue_visitor.cpp"
    "src/glsl/ir_stats.cpp"
    "src/glsl/ir_unused_structs.cpp"
    "src/glsl/ir_validate.cpp"
    "src/glsl/ir_variable_refcount.cpp"
    "src/glsl/link_atomics.cpp"
    "src/glsl/link_functions.cpp"
    "src/glsl/link_interface_blocks.cpp"
    "src/glsl/link_uniform_block_active_visitor.cpp"
    "src/glsl/link_uniform_blocks.cpp"
    "src/glsl/link_uniform_initializers.cpp"
    "src/glsl/link_uniforms.cpp"
    "src/glsl/link_varyings.cpp"
    "src/glsl/linker.cpp"
    "src/glsl/loop_analysis.cpp"
    "src/glsl/loop_controls.cpp"
    "src/glsl/loop_unroll.cpp"
    "src/glsl/lower_clip_distance.cpp"
    "src/glsl/lower_discard.cpp"
    "src/glsl/lower_discard_flow.cpp"
    "src/glsl/lower_if_to_cond_assign.cpp"
    "src/glsl/lower_instructions.cpp"
    "src/glsl/lower_jumps.cpp"
    "src/glsl/lower_mat_op_to_vec.cpp"
    "src/glsl/lower_named_interface_blocks.cpp"
    "src/glsl/lower_noise.cpp"
    "src/glsl/lower_offset_array.cpp"
    "src/glsl/lower_output_reads.cpp"
    "src/glsl/lower_packed_varyings.cpp"
    "src/glsl/lower_packing_builtins.cpp"
    "src/glsl/lower_ubo_reference.cpp"
    "src/glsl/lower_variable_index_to_cond_assign.cpp"
    "src/glsl/lower_vec_index_to_cond_assign.cpp"
    "src/glsl/lower_vec_index_to_swizzle.cpp"
    "src/glsl/lower_vector.cpp"
    "src/glsl/lower_vector_insert.cpp"
    "src/glsl/lower_vertex_id.cpp"
    "src/glsl/opt_algebraic.cpp"
    "src/glsl/opt_array_splitting.cpp"
    "src/glsl/opt_constant_folding.cpp"
    "src/glsl/opt_constant_propagation.cpp"
    "src/glsl/opt_constant_variable.cpp"
    "src/glsl/opt_copy_propagation.cpp"
    "src/glsl/opt_copy_propagation_elements.cpp"
    "src/glsl/opt_cse.cpp"
    "src/glsl/opt_dead_builtin_variables.cpp"
    "src/glsl/opt_dead_builtin_varyings.cpp"
    "src/glsl/opt_dead_code.cpp"
    "src/glsl/opt_dead_code_local.cpp"
    "src/glsl/opt_dead_functions.cpp"
    "src/glsl/opt_flatten_nested_if_blocks.cpp"
    "src/glsl/opt_flip_matrices.cpp"
    "src/glsl/opt_function_inlining.cpp"
    "src/glsl/opt_if_simplification.cpp"
    "src/glsl/opt_minmax.cpp"
    "src/glsl/opt_noop_swizzle.cpp"
    "src/glsl/opt_rebalance_tree.cpp"
    "src/glsl/opt_redundant_jumps.cpp"
    "src/glsl/opt_structure_splitting.cpp"
    "src/glsl/opt_swizzle_swizzle.cpp"
    "src/glsl/opt_tree_grafting.cpp"
    "src/glsl/opt_vectorize.cpp"
    "src/glsl/s_expression.cpp"
    "src/glsl/standalone_scaffolding.cpp"
)
add_library(glsl_optimizer STATIC ${SRC})
target_compile_features(glsl_optimizer PRIVATE c_std_99)
# generated Flex scanner uses 'register', which clang rejects
set_source_files_properties(src/glsl/glsl_lexer.cpp PROPERTIES
    COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-register>"
)
target_include_directories(glsl_optimizer
    PUBLIC src/glsl
    PRIVATE include src src/mesa
)
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

option(BUILD_TESTING "Build the regression tests" ${GLSLOPT_TOP_LEVEL})
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
