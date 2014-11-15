cmake_minimum_required(VERSION 2.8)
project(glsl_optimizer)

include_directories(include)
include_directories(include/c99)
include_directories(src)
include_directories(src/mesa)
include_directories(src/mapi)
include_directories(src/glsl)

# derived from msvs2010 project

set(GLSL_OPTIMIZER_HEADER
	src/glsl/ast.h
	src/glsl/builtin_type_macros.h
	src/glsl/glsl_optimizer.h
	src/glsl/glsl_parser.h
	src/glsl/glsl_parser_extras.h
	src/glsl/glsl_symbol_table.h
	src/glsl/glsl_types.h
	src/glsl/ir.h
	src/glsl/ir_basic_block.h
	src/glsl/ir_builder.h
	src/glsl/ir_expression_flattening.h
	src/glsl/ir_function_inlining.h
	src/glsl/ir_hierarchical_visitor.h
	src/glsl/ir_optimization.h
	src/glsl/ir_print_glsl_visitor.h
	src/glsl/ir_print_metal_visitor.h
	src/glsl/ir_print_visitor.h
	src/glsl/ir_rvalue_visitor.h
	src/glsl/ir_stats.h
	src/glsl/ir_uniform.h
	src/glsl/ir_unused_structs.h
	src/glsl/ir_variable_refcount.h
	src/glsl/ir_visitor.h
	src/glsl/linker.h
	src/glsl/link_uniform_block_active_visitor.h
	src/glsl/link_varyings.h
	src/glsl/list.h
	src/glsl/loop_analysis.h
	src/glsl/program.h
	src/glsl/s_expression.h
	src/glsl/standalone_scaffolding.h
	src/glsl/strtod.h
	src/glsl/glcpp/glcpp-parse.h
	src/glsl/glcpp/glcpp.h
	src/mesa/program/hash_table.h
	src/mesa/program/prog_instruction.h
	src/mesa/program/prog_parameter.h
	src/mesa/program/prog_statevars.h
	src/mesa/program/symbol_table.h
	src/mesa/main/compiler.h
	src/mesa/main/config.h
	src/mesa/main/context.h
	src/mesa/main/core.h
	src/mesa/main/dd.h
	src/mesa/main/glheader.h
	src/mesa/main/glminimal.h
	src/mesa/main/imports.h
	src/mesa/main/macros.h
	src/mesa/main/mtypes.h
	src/mesa/main/simple_list.h
	src/util/hash_table.h
	src/util/ralloc.h
)

set(GLSL_OPTIMIZER_SRCS
	src/glsl/ast_array_index.cpp
	src/glsl/ast_expr.cpp
	src/glsl/ast_function.cpp
	src/glsl/ast_to_hir.cpp
	src/glsl/ast_type.cpp
	src/glsl/builtin_functions.cpp
	src/glsl/builtin_types.cpp
	src/glsl/builtin_variables.cpp
	src/glsl/glsl_lexer.cpp
	src/glsl/glsl_optimizer.cpp
	src/glsl/glsl_parser.cpp
	src/glsl/glsl_parser_extras.cpp
	src/glsl/glsl_symbol_table.cpp
	src/glsl/glsl_types.cpp
	src/glsl/hir_field_selection.cpp
	src/glsl/ir.cpp
	src/glsl/ir_basic_block.cpp
	src/glsl/ir_builder.cpp
	src/glsl/ir_clone.cpp
	src/glsl/ir_constant_expression.cpp
	src/glsl/ir_equals.cpp
	src/glsl/ir_expression_flattening.cpp
	src/glsl/ir_function.cpp
	src/glsl/ir_function_can_inline.cpp
	src/glsl/ir_function_detect_recursion.cpp
	src/glsl/ir_hierarchical_visitor.cpp
	src/glsl/ir_hv_accept.cpp
	src/glsl/ir_import_prototypes.cpp
	src/glsl/ir_print_glsl_visitor.cpp
	src/glsl/ir_print_metal_visitor.cpp
	src/glsl/ir_print_visitor.cpp
	src/glsl/ir_rvalue_visitor.cpp
	src/glsl/ir_stats.cpp
	src/glsl/ir_unused_structs.cpp
	src/glsl/ir_validate.cpp
	src/glsl/ir_variable_refcount.cpp
	src/glsl/link_atomics.cpp
	src/glsl/link_functions.cpp
	src/glsl/link_interface_blocks.cpp
	src/glsl/link_uniform_blocks.cpp
	src/glsl/link_uniform_block_active_visitor.cpp
	src/glsl/link_uniform_initializers.cpp
	src/glsl/link_uniforms.cpp
	src/glsl/linker.cpp
	src/glsl/link_varyings.cpp
	src/glsl/loop_analysis.cpp
	src/glsl/loop_controls.cpp
	src/glsl/loop_unroll.cpp
	src/glsl/lower_clip_distance.cpp
	src/glsl/lower_discard.cpp
	src/glsl/lower_discard_flow.cpp
	src/glsl/lower_if_to_cond_assign.cpp
	src/glsl/lower_instructions.cpp
	src/glsl/lower_jumps.cpp
	src/glsl/lower_mat_op_to_vec.cpp
	src/glsl/lower_named_interface_blocks.cpp
	src/glsl/lower_noise.cpp
	src/glsl/lower_offset_array.cpp
	src/glsl/lower_output_reads.cpp
	src/glsl/lower_packed_varyings.cpp
	src/glsl/lower_packing_builtins.cpp
	src/glsl/lower_ubo_reference.cpp
	src/glsl/lower_variable_index_to_cond_assign.cpp
	src/glsl/lower_vector_insert.cpp
	src/glsl/lower_vec_index_to_cond_assign.cpp
	src/glsl/lower_vec_index_to_swizzle.cpp
	src/glsl/lower_vector.cpp
	src/glsl/lower_vertex_id.cpp
	src/glsl/opt_algebraic.cpp
	src/glsl/opt_array_splitting.cpp
	src/glsl/opt_constant_folding.cpp
	src/glsl/opt_constant_propagation.cpp
	src/glsl/opt_constant_variable.cpp
	src/glsl/opt_copy_propagation.cpp
	src/glsl/opt_copy_propagation_elements.cpp
	src/glsl/opt_cse.cpp
	src/glsl/opt_dead_builtin_variables.cpp
	src/glsl/opt_dead_builtin_varyings.cpp
	src/glsl/opt_dead_code.cpp
	src/glsl/opt_dead_code_local.cpp
	src/glsl/opt_dead_functions.cpp
	src/glsl/opt_flatten_nested_if_blocks.cpp
	src/glsl/opt_flip_matrices.cpp
	src/glsl/opt_function_inlining.cpp
	src/glsl/opt_if_simplification.cpp
	src/glsl/opt_minmax.cpp
	src/glsl/opt_noop_swizzle.cpp
	src/glsl/opt_rebalance_tree.cpp
	src/glsl/opt_redundant_jumps.cpp
	src/glsl/opt_structure_splitting.cpp
	src/glsl/opt_swizzle_swizzle.cpp
	src/glsl/opt_tree_grafting.cpp
	src/glsl/opt_vectorize.cpp
	src/glsl/opt_vector_splitting.cpp
	src/glsl/s_expression.cpp
	src/glsl/standalone_scaffolding.cpp
	src/glsl/strtod.c
	src/glsl/glcpp/glcpp-lex.c
	src/glsl/glcpp/glcpp-parse.c
	src/glsl/glcpp/pp.c
	src/mesa/main/imports.c
	src/mesa/program/prog_hash_table.c
	src/mesa/program/symbol_table.c
	src/util/hash_table.c
	src/util/ralloc.c
)

if(NOT MSVC)
	option(GLSL_OPTIMIZER_DEBUG "Enable debugging" FALSE)
	
	if(${GLSL_OPTIMIZER_DEBUG})
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
	else(${GLSL_OPTIMIZER_DEBUG})
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -DNDEBUG")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -DNDEBUG")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")
	endif(${GLSL_OPTIMIZER_DEBUG})
else(NOT MSVC)
	set(CMAKE_DEBUG_POSTFIX "_d")
endif(NOT MSVC)

add_library(glsl_optimizer ${GLSL_OPTIMIZER_HEADER} ${GLSL_OPTIMIZER_SRCS})

find_package(OpenGL REQUIRED)
add_executable(glsl_optimizer_tests tests/glsl_optimizer_tests.cpp)
target_link_libraries(glsl_optimizer_tests glsl_optimizer)
include_directories(glsl_optimizer_tests ${OPENGL_INCLUDE_DIRS})
target_link_libraries(glsl_optimizer_tests ${OPENGL_LIBRARIES})

if(MSVC)
	set_target_properties(glsl_optimizer PROPERTIES COMPILE_FLAGS /Fd"$(IntDir)$(TargetName).pdb")
	
	INSTALL(TARGETS glsl_optimizer DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/Debug CONFIGURATIONS Debug)
	INSTALL(FILES ${PROJECT_BINARY_DIR}/Debug/glsl_optimizer_d.pdb DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/Debug CONFIGURATIONS Debug)
	INSTALL(TARGETS glsl_optimizer DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/MinSizeRel CONFIGURATIONS MinSizeRel)
	INSTALL(TARGETS glsl_optimizer DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/Release CONFIGURATIONS Release)
	INSTALL(TARGETS glsl_optimizer DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/RelWithDebInfo CONFIGURATIONS RelWithDebInfo)
	#INSTALL(FILES ${PROJECT_BINARY_DIR}/RelWithDebInfo/glsl_optimizer.pdb DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/RelWithDebInfo CONFIGURATIONS RelWithDebInfo)
	INSTALL(FILES ${CMAKE_SOURCE_DIR}/src/glsl/glsl_optimizer.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif(MSVC)
