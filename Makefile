CSRCS = symbol_table.c hash_table.c glsl_types.c 
CCSRCS = glsl_parser.tab.cpp glsl_lexer.cpp glsl_parser_extras.cpp \
	ast_expr.cpp
#	ast_to_hir.cpp ir.cpp hir_field_selection.cpp
OBJS = $(CSRCS:.c=.o)  $(CCSRCS:.cpp=.o)

CC = gcc
CXX = g++
WARN     = -Wall -Wextra -Wunsafe-loop-optimizations -Wstack-protector \
	-Wunreachable-code
CPPFLAGS = -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE
CFLAGS   = -O0 -ggdb3 -fstack-protector $(CPPFLAGS) $(WARN) -std=c89 -ansi -pedantic
CXXFLAGS = -O0 -ggdb3 -fstack-protector $(CPPFLAGS) $(WARN)
LDLAGS   = -ggdb3

glsl: $(OBJS)
	$(CXX) $(LDLAGS) $(OBJS) -o glsl

glsl_parser.tab.cpp glsl_parser.tab.h: glsl_parser.y
	bison --report-file=glsl_parser.output -v -d \
	    --output=glsl_parser.tab.cpp \
	    --name-prefix=_mesa_glsl_ $< && \
	mv glsl_parser.tab.hpp glsl_parser.tab.h

glsl_lexer.cpp: glsl_lexer.l
	flex --outfile="glsl_lexer.cpp" $<

glsl_parser_tab.o: glsl_parser.tab.cpp
glsl_types.o: glsl_types.c glsl_types.h builtin_types.h
glsl_lexer.o: glsl_lexer.cpp glsl_parser.tab.h glsl_parser_extras.h ast.h
glsl_parser.o:  glsl_parser_extras.h ast.h
ast_to_hir.o: ast_to_hir.cpp symbol_table.h glsl_parser_extras.h ast.h glsl_types.h ir.h

builtin_types.h: builtin_types.sh
	./builtin_types.sh > builtin_types.h

clean:
	rm -f $(OBJS) glsl
	rm -f glsl_lexer.cpp glsl_parser.tab.{cpp,h,hpp} glsl_parser.output
	rm -f builtin_types.h
	rm -f *~