glcpp: glcpp.o glcpp-lex.o glcpp-parse.o

%.c %.h: %.y
	bison --defines=$*.h --output=$*.c $^

%.c: %.l
	flex --outfile=$@ $<

glcpp-lex.c: glcpp-parse.h

clean:
	rm -f glcpp-lex.c glcpp-parse.c *.o *~
