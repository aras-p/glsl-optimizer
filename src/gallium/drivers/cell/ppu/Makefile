# Gallium3D Cell driver: PPU code

# This makefile builds the libcell.a library which gets pulled into
# the main libGL.so library


TOP = ../../../../..
include $(TOP)/configs/current


# This is the "top-level" cell PPU driver code, will get pulled into libGL.so
# by the winsys Makefile.
CELL_LIB = ../libcell.a


# This is the SPU code.  We'd like to be able to put this into the libcell.a
# archive with the PPU code, but nesting .a libs doesn't seem to work.
# So, it's pulled into libGL.so in gallium/winsys/xlib/Makefile
SPU_CODE_MODULE = ../spu/g3d_spu.a


SOURCES = \
	cell_batch.c \
	cell_clear.c \
	cell_context.c \
	cell_draw_arrays.c \
	cell_fence.c \
	cell_flush.c \
	cell_gen_fragment.c \
	cell_gen_fp.c \
	cell_state_derived.c \
	cell_state_emit.c \
	cell_state_shader.c \
	cell_pipe_state.c \
	cell_screen.c \
	cell_state_vertex.c \
	cell_spu.c \
	cell_surface.c \
	cell_texture.c \
	cell_vbuf.c \
	cell_vertex_fetch.c \
	cell_vertex_shader.c


OBJECTS = $(SOURCES:.c=.o) \

INCLUDE_DIRS = \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@


.c.s:
	$(CC) -S $(INCLUDE_DIRS) $(CFLAGS) $< -o $@


default: $(CELL_LIB)


$(CELL_LIB): $(OBJECTS) $(SPU_CODE_MODULE)
#	ar -ru $(CELL_LIB) $(OBJECTS) $(SPU_CODE_MODULE) # doesn't work
	ar -ru $(CELL_LIB) $(OBJECTS)

#$(PROG): $(PPU_OBJECTS)
#	$(CC) -o $(PROG) $(PPU_OBJECTS) $(SPU_CODE_MODULE) $(PPU_LFLAGS)



clean:
	rm -f *.o *~ $(CELL_LIB)



depend: $(SOURCES)
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDE_DIRS) $(SOURCES) 2> /dev/null

include depend



