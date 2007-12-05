# Gallium3D Cell driver: PPU code

# This makefile builds the g3dcell.a library which gets pulled into
# the main libGL.so library


TOP = ../../../../..
include $(TOP)/configs/linux-cell


#PROG = gl4

CELL_LIB = libcell.a

SPU_CODE_MODULE = ../spu/g3d_spu.a


SOURCES = \
	cell_context.c \
	cell_surface.c

OBJECTS = $(SOURCES:.c=.o) \

INCLUDE_DIRS = -I$(TOP)/src/mesa


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(CELL_LIB)


$(CELL_LIB): $(OBJECTS) $(SPU_CODE_MODULE)
#	ar -ru $(CELL_LIB) $(OBJECTS) $(SPU_CODE_MODULE)
	ar -ru $(CELL_LIB) $(OBJECTS)

#$(PROG): $(PPU_OBJECTS)
#	$(CC) -o $(PROG) $(PPU_OBJECTS) $(SPU_CODE_MODULE) $(PPU_LFLAGS)



clean:
	rm -f *.o $(CELL_LIB)



depend: $(SOURCES)
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDE_DIRS) $(SOURCES) 2> /dev/null

include depend



