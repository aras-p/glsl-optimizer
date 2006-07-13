# progs/directfb/Makefile

TOP = ../..
include $(TOP)/configs/current


INCDIR = $(TOP)/include -I$(TOP)/progs

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME)

CFLAGS += $(shell pkg-config --cflags directfb)
APP_LIB_DEPS += $(shell pkg-config --libs directfb)

PROGS = df_gears \
	   df_reflect \
	   df_morph3d \
	   multi_window

##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c: $(LIB_DEP)
	$(CC) -I$(INCDIR) $(CFLAGS) $< $(APP_LIB_DEPS) -o $@


##### TARGETS #####

default: $(PROGS)


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~

