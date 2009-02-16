# Gallium3D Cell driver: SPU code

# This makefile builds the g3d_spu.a file that's linked into the
# PPU code/library.


TOP = ../../../../..
include $(TOP)/configs/current


PROG = g3d

PROG_SPU = $(PROG)_spu
PROG_SPU_A = $(PROG)_spu.a
PROG_SPU_EMBED_O = $(PROG)_spu-embed.o


SOURCES = \
	spu_command.c \
	spu_dcache.c \
	spu_funcs.c \
	spu_main.c \
	spu_per_fragment_op.c \
	spu_render.c \
	spu_texture.c \
	spu_tile.c \
	spu_tri.c

OLD_SOURCES = \
	spu_exec.c \
	spu_util.c \
	spu_vertex_fetch.c \
	spu_vertex_shader.c


SPU_OBJECTS = $(SOURCES:.c=.o)

SPU_ASM_OUT = $(SOURCES:.c=.s)


INCLUDE_DIRS = \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers


.c.o:
	$(SPU_CC) $(SPU_CFLAGS) -c $<

.c.s:
	$(SPU_CC) $(SPU_CFLAGS) -O3 -S $<


# The .a file will be linked into the main/PPU executable
default: $(PROG_SPU_A)

$(PROG_SPU_A): $(PROG_SPU_EMBED_O)
	$(SPU_AR) $(SPU_AR_FLAGS) $(PROG_SPU_A) $(PROG_SPU_EMBED_O)

$(PROG_SPU_EMBED_O): $(PROG_SPU)
	$(SPU_EMBED) $(SPU_EMBED_FLAGS) $(PROG_SPU) $(PROG_SPU) $(PROG_SPU_EMBED_O)

$(PROG_SPU): $(SPU_OBJECTS)
	$(SPU_CC) -o $(PROG_SPU) $(SPU_OBJECTS) $(SPU_LFLAGS)



asmfiles: $(SPU_ASM_OUT)


clean:
	rm -f *~ *.o *.a *.d *.s $(PROG_SPU)



depend: $(SOURCES)
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDE_DIRS) $(SOURCES) 2> /dev/null

include depend

