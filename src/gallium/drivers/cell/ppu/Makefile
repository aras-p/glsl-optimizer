# Gallium3D Cell driver: PPU code

# This makefile builds the g3dcell.a library which gets pulled into
# the main libGL.so library


TOP = ../../../../..
include $(TOP)/configs/linux-cell


#PROG = gl4

CELL_LIB = libcell.a

SPU_CODE_MODULE = ../spu/g3d_spu.a


SOURCES = \
	cell_batch.c \
	cell_clear.c \
	cell_context.c \
	cell_draw_arrays.c \
	cell_flush.c \
	cell_state_blend.c \
	cell_state_clip.c \
	cell_state_derived.c \
	cell_state_emit.c \
	cell_state_fs.c \
	cell_state_rasterizer.c \
	cell_state_sampler.c \
	cell_state_surface.c \
	cell_state_vertex.c \
	cell_spu.c \
	cell_surface.c \
	cell_texture.c \
	cell_vbuf.c \
	cell_vertex_shader.c \
	cell_winsys.c


OBJECTS = $(SOURCES:.c=.o) \

INCLUDE_DIRS = \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(CELL_LIB)


$(CELL_LIB): $(OBJECTS) $(SPU_CODE_MODULE)
#	ar -ru $(CELL_LIB) $(OBJECTS) $(SPU_CODE_MODULE)
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



