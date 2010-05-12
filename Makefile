# Debug symbols by default, but let the user avoid that with something
# like "make CFLAGS=-O2"
CFLAGS = -g

# But we use 'override' here so that "make CFLAGS=-O2" will still have
# all the warnings enabled.
override CFLAGS += -Wall -Wextra -Wwrite-strings -Wswitch-enum -Wno-unused

glcpp: glcpp.o glcpp-lex.o glcpp-parse.o hash_table.o
	gcc -o $@ -ltalloc $^

%.c %.h: %.y
	bison --debug --defines=$*.h --output=$*.c $^

%.c: %.l
	flex --outfile=$@ $<

glcpp-lex.c: glcpp-parse.h

test:
	@(cd tests; ./glcpp-test)

clean:
	rm -f glcpp-lex.c glcpp-parse.c *.o *~
	rm -f tests/*.out tests/*.gcc tests/*.expected
