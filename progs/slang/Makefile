# progs/slang/Makefile

TOP = ../..

include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)

#
# targets
#

.PHONY: default tests clean

default: cltest sotest vstest

tests: default
	! ./sotest 2> /dev/null | (grep -e ^[*][*][*])
	! ./vstest 2> /dev/null | (grep -e ^[*][*][*])
	! ./cltest 2> /dev/null | (grep -e ^[*][*][*])

clean:
	rm -f cltest.o sotest.o vstest.o framework.o cltest sotest vstest

#
# executables
#

cltest: cltest.o framework.o $(LIB_DEP)
	$(CC) cltest.o framework.o $(APP_LIB_DEPS) -o cltest

sotest: sotest.o framework.o $(LIB_DEP)
	$(CC) sotest.o framework.o $(APP_LIB_DEPS) -o sotest

vstest: vstest.o framework.o $(LIB_DEP)
	$(CC) vstest.o framework.o $(APP_LIB_DEPS) -o vstest

#
# objects
#

framework.o: framework.c framework.h
	$(CC) -c -I$(INCDIR) framework.c -o framework.o

cltest.o: cltest.c framework.h
	$(CC) -c -I$(INCDIR) cltest.c -o cltest.o

sotest.o: sotest.c framework.h
	$(CC) -c -I$(INCDIR) sotest.c -o sotest.o

vstest.o: vstest.c framework.h
	$(CC) -c -I$(INCDIR) vstest.c -o vstest.o

