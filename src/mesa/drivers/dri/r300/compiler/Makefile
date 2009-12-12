# src/mesa/drivers/dri/r300/compiler/Makefile

TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = r300compiler

C_SOURCES = \
		radeon_code.c \
		radeon_compiler.c \
		radeon_program.c \
		radeon_program_print.c \
		radeon_opcodes.c \
		radeon_program_alu.c \
		radeon_program_pair.c \
		radeon_pair_translate.c \
		radeon_pair_schedule.c \
		radeon_pair_regalloc.c \
		radeon_dataflow.c \
		radeon_dataflow_deadcode.c \
		radeon_dataflow_swizzles.c \
		r3xx_fragprog.c \
		r300_fragprog.c \
		r300_fragprog_swizzle.c \
		r300_fragprog_emit.c \
		r500_fragprog.c \
		r500_fragprog_emit.c \
		r3xx_vertprog.c \
		r3xx_vertprog_dump.c \
		\
		memory_pool.c


### Basic defines ###

OBJECTS = $(C_SOURCES:.c=.o) \
	$(CPP_SOURCES:.cpp=.o) \
	$(ASM_SOURCES:.S=.o)

INCLUDES = \
	-I. \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \


##### TARGETS #####

default: depend lib$(LIBNAME).a

lib$(LIBNAME).a: $(OBJECTS) Makefile $(TOP)/configs/current
	$(MKLIB) -o $(LIBNAME) -static $(OBJECTS)

depend: $(C_SOURCES) $(CPP_SOURCES) $(ASM_SOURCES) $(SYMLINKS)
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(C_SOURCES) $(CPP_SOURCES) $(ASM_SOURCES) 2> /dev/null

# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`

# Remove .o and backup files
clean:
	rm -f $(OBJECTS) lib$(LIBNAME).a depend depend.bak

# Dummy target
install:
	@echo -n ""

##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(LIBRARY_DEFINES) $< -o $@

.cpp.o:
	$(CXX) -c $(INCLUDES) $(CXXFLAGS) $(LIBRARY_DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(LIBRARY_DEFINES)  $< -o $@


sinclude depend
