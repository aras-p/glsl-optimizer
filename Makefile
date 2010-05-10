override CFLAGS += -Wall -Wextra -Wwrite-strings -Wswitch-enum -Wno-unused

glcpp: glcpp.o glcpp-lex.o glcpp-parse.o hash_table.o

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
