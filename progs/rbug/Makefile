# progs/rbug/Makefile

TOP = ../..
include $(TOP)/configs/current

INCLUDES = \
	-I. \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/gallium/drivers \
	$(PROG_INCLUDES)

LINKS = \
	$(GALLIUM_AUXILIARIES) \
	$(PROG_LINKS)

SOURCES = \
	bin_to_bmp.c \
	simple_client.c \
	simple_server.c \
	shdr_info.c \
	shdr_dump.c \
	shdr_disable.c \
	ctx_info.c \
	ctx_rule.c \
	tex_info.c \
	tex_dump.c


OBJECTS = $(SOURCES:.c=.o)

PROGS = $(OBJECTS:.o=)

##### TARGETS #####

default: $(OBJECTS) $(PROGS)

clean:
	-rm -f $(PROGS)
	-rm -f *.o

##### RULES #####

$(OBJECTS): %.o: %.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $(PROG_DEFINES) $< -o $@

$(PROGS): %: %.o
	$(CC) $(LDFLAGS) $< $(LINKS) -o $@
