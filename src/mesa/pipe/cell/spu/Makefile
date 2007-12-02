# Gallium3D Cell driver: PPU code

# This makefile builds the g3d_spu.a file that's linked into the
# PPU code/library.


TOP = ../../../../..
include $(TOP)/configs/linux-cell


PROG = g3d

PROG_SPU = $(PROG)_spu
PROG_SPU_A = $(PROG)_spu.a
PROG_SPU_EMBED_O = $(PROG)_spu-embed.o


SPU_OBJECTS = main.o tri.o


# The .a file will be linked into the main/PPU executable
default: $(PROG_SPU_A)

$(PROG_SPU_A): $(PROG_SPU_EMBED_O)
	$(SPU_AR) $(SPU_AR_FLAGS) $(PROG_SPU_A) $(PROG_SPU_EMBED_O)

$(PROG_SPU_EMBED_O): $(PROG_SPU)
	$(SPU_EMBED) $(SPU_EMBED_FLAGS) $(PROG_SPU) $(PROG_SPU) $(PROG_SPU_EMBED_O)

$(PROG_SPU): $(SPU_OBJECTS)
	$(SPU_CC) -o $(PROG_SPU) $(SPU_OBJECTS) $(SPU_LFLAGS)


main.o: main.c
	$(SPU_CC) $(SPU_CFLAGS) -c main.c

tri.o: tri.c
	$(SPU_CC) $(SPU_CFLAGS) -c tri.c


clean:
	rm -f *.o *.a *.d $(PROG_SPU)
