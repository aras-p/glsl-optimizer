# src/gallium/tests/trivial/Makefile

TOP = ../../../..
include $(TOP)/configs/current

INCLUDES = \
	-I. \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/winsys \
	$(PROG_INCLUDES)

ifeq ($(MESA_LLVM),1)
LINKS = $(TOP)/src/gallium/drivers/llvmpipe/libllvmpipe.a
LDFLAGS += $(LLVM_LDFLAGS)
endif

LINKS += \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/galahad/libgalahad.a \
	$(TOP)/src/gallium/winsys/sw/null/libws_null.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(GALLIUM_AUXILIARIES) \
	$(PROG_LINKS)

SOURCES = \
	tri.c \
	quad-tex.c

OBJECTS = $(SOURCES:.c=.o)

PROGS = $(OBJECTS:.o=)

PROG_DEFINES = \
	-DGALLIUM_SOFTPIPE -DGALLIUM_RBUG -DGALLIUM_TRACE -DGALLIUM_GALAHAD

##### TARGETS #####

default: $(PROGS)

clean:
	-rm -f $(PROGS)
	-rm -f *.o
	-rm -f result.bmp

##### RULES #####

$(OBJECTS): %.o: %.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $(PROG_DEFINES) $< -o $@

$(PROGS): %: %.o $(LINKS)
	$(CXX) $(LDFLAGS) $< $(LINKS) $(LLVM_LIBS) -lm -lpthread -ldl -o $@
