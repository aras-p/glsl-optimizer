# progs/slang/Makefile

TOP = ../..

include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

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
	-rm -f cltest.o sotest.o vstest.o framework.o cltest sotest vstest

#
# executables
#

cltest: cltest.o framework.o $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) cltest.o framework.o $(LIBS) -o cltest

sotest: sotest.o framework.o $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) sotest.o framework.o $(LIBS) -o sotest

vstest: vstest.o framework.o $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) vstest.o framework.o $(LIBS) -o vstest

#
# objects
#

framework.o: framework.c framework.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) framework.c -o framework.o

cltest.o: cltest.c framework.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) cltest.c -o cltest.o

sotest.o: sotest.c framework.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) sotest.c -o sotest.o

vstest.o: vstest.c framework.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) vstest.c -o vstest.o

