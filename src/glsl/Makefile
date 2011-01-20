#src/glsl/pp/Makefile

TOP = ../..

include $(TOP)/configs/current

LIBNAME = glsl

LIBGLCPP_SOURCES = \
	glcpp/glcpp-lex.c \
	glcpp/glcpp-parse.c \
	glcpp/pp.c

GLCPP_SOURCES = \
	$(LIBGLCPP_SOURCES) \
	glcpp/glcpp.c

C_SOURCES = \
	strtod.c \
	$(LIBGLCPP_SOURCES)

CXX_SOURCES = \
	ast_expr.cpp \
	ast_function.cpp \
	ast_to_hir.cpp \
	ast_type.cpp \
	glsl_lexer.cpp \
	glsl_parser.cpp \
	glsl_parser_extras.cpp \
	glsl_types.cpp \
	glsl_symbol_table.cpp \
	hir_field_selection.cpp \
	ir_basic_block.cpp \
	ir_clone.cpp \
	ir_constant_expression.cpp \
	ir.cpp \
	ir_expression_flattening.cpp \
	ir_function_can_inline.cpp \
	ir_function.cpp \
	ir_hierarchical_visitor.cpp \
	ir_hv_accept.cpp \
	ir_import_prototypes.cpp \
	ir_print_visitor.cpp \
	ir_reader.cpp \
	ir_rvalue_visitor.cpp \
	ir_set_program_inouts.cpp \
	ir_validate.cpp \
	ir_variable.cpp \
	ir_variable_refcount.cpp \
	linker.cpp \
	link_functions.cpp \
	loop_analysis.cpp \
	loop_controls.cpp \
	loop_unroll.cpp \
	lower_discard.cpp \
	lower_if_to_cond_assign.cpp \
	lower_instructions.cpp \
	lower_jumps.cpp \
	lower_mat_op_to_vec.cpp \
	lower_noise.cpp \
	lower_texture_projection.cpp \
	lower_variable_index_to_cond_assign.cpp \
	lower_vec_index_to_cond_assign.cpp \
	lower_vec_index_to_swizzle.cpp \
	lower_vector.cpp \
	opt_algebraic.cpp \
	opt_constant_folding.cpp \
	opt_constant_propagation.cpp \
	opt_constant_variable.cpp \
	opt_copy_propagation.cpp \
	opt_dead_code.cpp \
	opt_dead_code_local.cpp \
	opt_dead_functions.cpp \
	opt_discard_simplification.cpp \
	opt_function_inlining.cpp \
	opt_if_simplification.cpp \
	opt_noop_swizzle.cpp \
	opt_redundant_jumps.cpp \
	opt_structure_splitting.cpp \
	opt_swizzle_swizzle.cpp \
	opt_tree_grafting.cpp \
	s_expression.cpp

LIBS = \
	$(TOP)/src/glsl/libglsl.a \
	$(TALLOC_LIBS)

APPS = glsl_compiler glcpp/glcpp

GLSL2_C_SOURCES = \
	../mesa/program/hash_table.c \
	../mesa/program/symbol_table.c
GLSL2_CXX_SOURCES = \
	main.cpp

GLSL2_OBJECTS = \
	$(GLSL2_C_SOURCES:.c=.o) \
	$(GLSL2_CXX_SOURCES:.cpp=.o)

### Basic defines ###

DEFINES += \
	$(LIBRARY_DEFINES) \
	$(API_DEFINES)

GLCPP_OBJECTS = \
	$(GLCPP_SOURCES:.c=.o) \
	../mesa/program/hash_table.o

OBJECTS = \
	$(C_SOURCES:.c=.o) \
	$(CXX_SOURCES:.cpp=.o)

INCLUDES = \
	$(TALLOC_CFLAGS) \
	-I. \
	-I../mesa \
	-I../mapi \
	-I../../include \
	$(LIBRARY_INCLUDES)

ALL_SOURCES = \
	$(C_SOURCES) \
	$(CXX_SOURCES) \
	$(GLSL2_CXX_SOURCES) \
	$(GLSL2_C_SOURCES)

##### TARGETS #####

default: depend lib$(LIBNAME).a $(APPS)

lib$(LIBNAME).a: $(OBJECTS) builtin_function.o Makefile $(TOP)/src/glsl/Makefile.template
	$(MKLIB) -cplusplus -o $(LIBNAME) -static $(OBJECTS) builtin_function.o

depend: $(ALL_SOURCES) Makefile
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(ALL_SOURCES) 2> /dev/null

# Remove .o and backup files
clean:
	rm -f $(GLCPP_OBJECTS) $(GLSL2_OBJECTS) $(OBJECTS) lib$(LIBNAME).a depend depend.bak builtin_function.cpp builtin_function.o builtin_stubs.o builtin_compiler
	-rm -f $(APPS)

# Dummy target
install:
	@echo -n ""


##### RULES #####

glsl_compiler: $(GLSL2_OBJECTS) libglsl.a
	$(APP_CXX) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(GLSL2_OBJECTS) $(LIBS) -o $@

glcpp/glcpp: $(GLCPP_OBJECTS) libglsl.a
	$(APP_CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(GLCPP_OBJECTS) $(LIBS) -o $@

.cpp.o:
	$(CXX) -c $(INCLUDES) $(CXXFLAGS) $(DEFINES) $< -o $@

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

glsl_lexer.cpp: glsl_lexer.lpp
	flex --nounistd -o$@  $<

glsl_parser.cpp: glsl_parser.ypp
	bison -v -o "$@" -p "_mesa_glsl_" --defines=glsl_parser.h $<

glcpp/glcpp-lex.c: glcpp/glcpp-lex.l
	flex --nounistd -o$@  $<

glcpp/glcpp-parse.c: glcpp/glcpp-parse.y
	bison -v -o "$@" --defines=glcpp/glcpp-parse.h $<

builtin_compiler: $(GLSL2_OBJECTS) $(OBJECTS) builtin_stubs.o
	$(APP_CXX) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(OBJECTS) $(GLSL2_OBJECTS) builtin_stubs.o $(TALLOC_LIBS) -o $@

builtin_function.cpp: builtins/profiles/* builtins/ir/* builtins/tools/generate_builtins.py builtins/tools/texture_builtins.py builtin_compiler
	@echo Regenerating builtin_function.cpp...
	$(PYTHON2) $(PYTHON_FLAGS) builtins/tools/generate_builtins.py $(PWD)/builtin_compiler > builtin_function.cpp

-include depend
