override CFLAGS += -Wall -Wextra -Wwrite-strings -Wswitch-enum -Wno-unused

glcpp: glcpp.o glcpp-lex.o glcpp-parse.o hash_table.o

%.c %.h: %.y
	bison --debug --defines=$*.h --output=$*.c $^

%.c: %.l
	flex --outfile=$@ $<

glcpp-lex.c: glcpp-parse.h

clean:
	rm -f glcpp-lex.c glcpp-parse.c *.o *~
