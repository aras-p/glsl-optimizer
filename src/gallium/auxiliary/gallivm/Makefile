# -*-makefile-*-
TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = gallivm


GALLIVM_SOURCES = \
        gallivm.cpp  \
        gallivm_cpu.cpp \
        instructions.cpp  \
        loweringpass.cpp \
        tgsitollvm.cpp \
        storage.cpp \
        storagesoa.cpp \
        instructionssoa.cpp

INC_SOURCES = gallivm_builtins.cpp gallivmsoabuiltins.cpp

CPP_SOURCES = \
	$(GALLIVM_SOURCES)

C_SOURCES =
ASM_SOURCES =

OBJECTS = $(C_SOURCES:.c=.o) \
          $(CPP_SOURCES:.cpp=.o) \
	  $(ASM_SOURCES:.S=.o)

### Include directories
INCLUDES = \
	-I. \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/include


##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(LLVM_CFLAGS) $(CFLAGS) $(DRIVER_DEFINES) $< -o $@

.cpp.o:
	$(CXX) -c $(INCLUDES) $(LLVM_CXXFLAGS) $(CXXFLAGS) $(DRIVER_DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DRIVER_DEFINES)  $< -o $@

##### TARGETS #####

default:: depend symlinks $(LIBNAME)


$(LIBNAME): $(OBJECTS) Makefile
	$(TOP)/bin/mklib -o $@ -static $(OBJECTS)


depend: $(C_SOURCES) $(CPP_SOURCES) $(ASM_SOURCES) $(INC_SOURCES)
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DRIVER_DEFINES) $(INCLUDES) $(C_SOURCES) $(CPP_SOURCES) \
		$(ASM_SOURCES) $(INC_SOURCES) 2> /dev/null


gallivm_builtins.cpp: llvm_builtins.c
	clang --emit-llvm < $< |llvm-as|opt -std-compile-opts > temp1.bin
	(echo "static const unsigned char llvm_builtins_data[] = {"; od -txC temp1.bin | sed -e "s/^[0-9]*//" -e s"/ \([0-9a-f][0-9a-f]\)/0x\1,/g" -e"\$$d" | sed -e"\$$s/,$$/};/") >$@
	rm temp1.bin

gallivmsoabuiltins.cpp: soabuiltins.c
	clang --emit-llvm < $< |llvm-as|opt -std-compile-opts > temp2.bin
	(echo "static const unsigned char soabuiltins_data[] = {"; od -txC temp2.bin | sed -e "s/^[0-9]*//" -e s"/ \([0-9a-f][0-9a-f]\)/0x\1,/g" -e"\$$d" | sed -e"\$$s/,$$/};/") >$@
	rm temp2.bin

# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


# Remove .o and backup files
clean:
	-rm -f *.o */*.o *~ *.so *~ server/*.o
	-rm -f depend depend.bak
	-rm -f gallivm_builtins.cpp
	-rm -f gallivmsoabuiltins.cpp

symlinks:


include depend
