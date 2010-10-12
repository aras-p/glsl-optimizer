all: program_parse.tab.c lex.yy.c

program_parse.tab.c program_parse.tab.h: program_parse.y
	bison -v -d $<

lex.yy.c: program_lexer.l
	flex --never-interactive $<
