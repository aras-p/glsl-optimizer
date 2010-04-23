TOP = ../..
MAPI = $(TOP)/src/mapi
include $(TOP)/configs/current

include $(MAPI)/glapi/sources.mak
GLESv1_CM_ASM := $(addprefix $(MAPI)/es1api/glapi/,$(GLAPI_ASM_SOURCES))
GLESv2_ASM := $(addprefix $(MAPI)/es2api/glapi/,$(GLAPI_ASM_SOURCES))
API_SOURCES := $(addprefix $(MAPI)/glapi/,$(GLAPI_SOURCES))

include $(MAPI)/mapi/sources.mak
MAPI_GLAPI_SOURCES := $(addprefix $(MAPI)/mapi/,$(MAPI_GLAPI_SOURCES))

$(TOP)/$(LIB_DIR)/$(GLESv1_CM_LIB_NAME) : PREFIX = es1
$(TOP)/$(LIB_DIR)/$(GLESv1_CM_LIB_NAME) : NAME = GLESv1_CM

$(TOP)/$(LIB_DIR)/$(GLESv2_LIB_NAME) : PREFIX = es2
$(TOP)/$(LIB_DIR)/$(GLESv2_LIB_NAME) : NAME = GLESv2

INCLUDES = -I$(TOP)/include -I$(MAPI)/$(PREFIX)api -I$(MAPI) -I$(TOP)/src/mesa

OBJECTS = \
	$(notdir $(GLAPI_ASM_SOURCES:%.S=%.o)) \
	$(notdir $(GLAPI_SOURCES:%.c=%.o)) \
	$(notdir $(MAPI_GLAPI_SOURCES:%.c=%.o))

GLESv1_CM_OBJECTS = $(addprefix es1-,$(OBJECTS))
GLESv2_OBJECTS = $(addprefix es2-,$(OBJECTS))

es1-%.o: $(dir $(GLESv1_CM_ASM))%.S
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<
es1-%.o: $(MAPI)/glapi/%.c
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<
es1-u_%.o: $(MAPI)/mapi/u_%.c
	$(CC) -c $(CFLAGS) $(INCLUDES) -DMAPI_GLAPI_CURRENT -o $@ $<

es2-%.o: $(dir $(GLESv2_ASM))%.S
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<
es2-%.o: $(MAPI)/glapi/%.c
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<
es2-u_%.o: $(MAPI)/mapi/u_%.c
	$(CC) -c $(CFLAGS) $(INCLUDES) -DMAPI_GLAPI_CURRENT -o $@ $<

default: depend \
	$(TOP)/$(LIB_DIR)/$(GLESv1_CM_LIB_NAME) \
	$(TOP)/$(LIB_DIR)/$(GLESv2_LIB_NAME)

$(TOP)/$(LIB_DIR)/$(GLESv1_CM_LIB_NAME) : $(GLESv1_CM_OBJECTS)
$(TOP)/$(LIB_DIR)/$(GLESv2_LIB_NAME) : $(GLESv2_OBJECTS)

$(TOP)/$(LIB_DIR)/$(GLESv1_CM_LIB_NAME) \
$(TOP)/$(LIB_DIR)/$(GLESv2_LIB_NAME) : Makefile
	$(MKLIB) -o $($(NAME)_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major 1 -minor 2 $(MKLIB_OPTIONS) \
		-install $(TOP)/$(LIB_DIR) -id \
		$(INSTALL_LIB_DIR)/lib$($(NAME)_LIB).1.dylib \
		$($(NAME)_LIB_DEPS) $($(NAME)_OBJECTS)

depend: Makefile
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) -DMAPI_GLAPI_CURRENT \
		$(API_SOURCES) $(ES1_API_ASM)  $(ES2_API_ASM)

# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find $(TOP)/include`

install: $(TOP)/$(LIB_DIR)/$(GLESv1_CM_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLESv2_LIB_NAME)
	$(MAKE) -C $(TOP)/src/mesa install-es2 install-es1

# Remove .o and backup files
clean:
	-rm -f $(TOP)/$(LIB_DIR)/$(GLESv1_CM_LIB_GLOB)
	-rm -f $(TOP)/$(LIB_DIR)/$(GLESv2_LIB_GLOB)
	-rm -f *.o *~
	-rm -f depend depend.bak

-include depend
