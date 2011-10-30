
#src/glsl/pp/Makefile

TOP = ../..

include $(TOP)/configs/current

LIBNAME = glsl

include Makefile.sources

GLCPP_SOURCES = \
	$(LIBGLCPP_GENERATED_SOURCES) \
	$(LIBGLCPP_SOURCES) \
	ralloc.c \
	glcpp/glcpp.c

C_SOURCES = \
	$(LIBGLCPP_GENERATED_SOURCES) \
	$(LIBGLCPP_SOURCES) \
	$(LIBGLSL_SOURCES)

# common sources for builtin_compiler and libglsl
CXX_SOURCES = \
	$(BUILTIN_COMPILER_GENERATED_CXX_SOURCES) \
	$(LIBGLSL_CXX_SOURCES)

LIBS = \
	$(TOP)/src/glsl/libglsl.a

APPS = glsl_compiler glsl_test glcpp/glcpp

GLSL2_C_SOURCES = \
	../mesa/program/hash_table.c \
	../mesa/program/symbol_table.c
GLSL2_CXX_SOURCES = \
	$(GLSL_COMPILER_CXX_SOURCES)

GLSL2_OBJECTS = \
	$(GLSL2_C_SOURCES:.c=.o) \
	$(GLSL2_CXX_SOURCES:.cpp=.o)

TEST_C_SOURCES = \
	../mesa/program/hash_table.c \
	../mesa/program/symbol_table.c

TEST_CXX_SOURCES = \
	standalone_scaffolding.cpp \
	test.cpp \
	test_optpass.cpp

TEST_OBJECTS = \
	$(TEST_C_SOURCES:.c=.o) \
	$(TEST_CXX_SOURCES:.cpp=.o)

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

DRICORE_OBJ_DIR = obj-visible
OBJECTS_DRICORE = $(addprefix $(DRICORE_OBJ_DIR)/,$(OBJECTS))

INCLUDES = \
	-I. \
	-I../mesa \
	-I../mapi \
	-I../../include \
	$(LIBRARY_INCLUDES)

ALL_SOURCES = \
	$(C_SOURCES) \
	$(CXX_SOURCES) \
	$(GLSL2_CXX_SOURCES) \
	$(GLSL2_C_SOURCES) \
	$(TEST_CXX_SOURCES) \
	$(TEST_C_SOURCES)

##### TARGETS #####

default: depend lib$(LIBNAME).a $(APPS) $(DRICORE_GLSL_LIBS)

$(TOP)/$(LIB_DIR)/libglsl.so: $(OBJECTS_DRICORE) builtin_function.o Makefile $(TOP)/src/glsl/Makefile.template
	$(MKLIB) -o $@ -linker '$(CXX)' -ldflags '$(LDFLAGS)' \
		-cplusplus -noprefix \
		-install $(TOP)/$(LIB_DIR) -id $(INSTALL_LIB_DIR)/$@.dylib \
		$(OBJECTS_DRICORE) builtin_function.o

lib$(LIBNAME).a: $(OBJECTS) builtin_function.o Makefile $(TOP)/src/glsl/Makefile.template
	$(MKLIB) -cplusplus -o $(LIBNAME) -static $(OBJECTS) builtin_function.o

depend: $(ALL_SOURCES) Makefile
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(ALL_SOURCES) 2> /dev/null
	$(MKDEP) $(MKDEP_OPTIONS) -a -p $(DRICORE_OBJ_DIR)/ $(INCLUDES) $(ALL_SOURCES) 2> /dev/null

# Remove .o and backup files
clean: clean-dricore
	rm -f $(GLCPP_OBJECTS) $(GLSL2_OBJECTS) $(TEST_OBJECTS) $(OBJECTS) lib$(LIBNAME).a depend depend.bak builtin_function.cpp builtin_function.o builtin_stubs.o builtin_compiler
	-rm -f $(APPS)

clean-dricore:
	-rm -f $(OBJECTS_DRICORE) $(TOP)/$(LIB_DIR)/libglsl.so libglsl.so

ifneq (,$(DRICORE_GLSL_LIBS))
DRICORE_INSTALL_TARGET = install-dricore
endif

# Dummy target
install: $(DRICORE_INSTALL_TARGET)
	@echo -n ""

install-dricore: default
	$(INSTALL) -d $(DESTDIR)$(DRI_DRIVER_INSTALL_DIR)
	$(INSTALL) -m 755 $(DRICORE_GLSL_LIBS) $(DESTDIR)$(DRI_DRIVER_INSTALL_DIR)

##### RULES #####

glsl_compiler: $(GLSL2_OBJECTS) libglsl.a
	$(APP_CXX) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(GLSL2_OBJECTS) $(LIBS) -o $@

glsl_test: $(TEST_OBJECTS) libglsl.a
	$(APP_CXX) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(TEST_OBJECTS) $(LIBS) -o $@

glcpp: glcpp/glcpp
glcpp/glcpp: $(GLCPP_OBJECTS)
	$(APP_CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $(GLCPP_OBJECTS) -o $@

.cpp.o:
	$(CXX) -c $(INCLUDES) $(CXXFLAGS) $(DEFINES) $< -o $@

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

$(DRICORE_OBJ_DIR)/%.o : %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(INCLUDES) $(DRI_CXXFLAGS) $(DEFINES) $< -o $@

$(DRICORE_OBJ_DIR)/%.o : %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(INCLUDES) $(DRI_CFLAGS) $(DEFINES) $< -o $@

glsl_lexer.cpp: glsl_lexer.ll
	$(FLEX) --nounistd -o$@  $<

glsl_parser.cpp: glsl_parser.yy
	$(BISON) -v -o "$@" -p "_mesa_glsl_" --defines=glsl_parser.h $<

glcpp/glcpp-lex.c: glcpp/glcpp-lex.l
	$(FLEX) --nounistd -o$@  $<

glcpp/glcpp-parse.c: glcpp/glcpp-parse.y
	$(BISON) -v -o "$@" --defines=glcpp/glcpp-parse.h $<

builtin_compiler: $(GLSL2_OBJECTS) $(OBJECTS) builtin_stubs.o
	$(APP_CXX) $(INCLUDES) $(CXXFLAGS) $(LDFLAGS) $(OBJECTS) $(GLSL2_OBJECTS) builtin_stubs.o -o $@

builtin_function.cpp: builtins/profiles/* builtins/ir/* builtins/tools/generate_builtins.py builtins/tools/texture_builtins.py builtin_compiler
	@echo Regenerating builtin_function.cpp...
	$(PYTHON2) $(PYTHON_FLAGS) builtins/tools/generate_builtins.py ./builtin_compiler > builtin_function.cpp || rm -f builtin_function.cpp

-include depend
