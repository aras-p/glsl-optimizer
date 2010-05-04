TOP = ../..
MESA = $(TOP)/src/mesa
include $(TOP)/configs/current
include $(MESA)/sources.mak

GLESv1_CM_ASM := $(addprefix $(MESA)/es/glapi/glapi-es1/,$(GLAPI_ASM_SOURCES))
GLESv2_ASM := $(addprefix $(MESA)/es/glapi/glapi-es2/,$(GLAPI_ASM_SOURCES))
API_SOURCES := $(addprefix $(MESA)/,$(GLAPI_SOURCES))

$(TOP)/$(LIB_DIR)/$(GLESv1_CM_LIB_NAME) : PREFIX = es1
$(TOP)/$(LIB_DIR)/$(GLESv1_CM_LIB_NAME) : NAME = GLESv1_CM

$(TOP)/$(LIB_DIR)/$(GLESv2_LIB_NAME) : PREFIX = es2
$(TOP)/$(LIB_DIR)/$(GLESv2_LIB_NAME) : NAME = GLESv2

INCLUDES = -I$(TOP)/include -I$(MESA)/es/glapi/glapi-$(PREFIX) -I$(MESA)

OBJECTS = \
	$(notdir $(GLAPI_ASM_SOURCES:%.S=%.o)) \
	$(notdir $(GLAPI_SOURCES:%.c=%.o))

GLESv1_CM_OBJECTS = $(addprefix es1-,$(OBJECTS))
GLESv2_OBJECTS = $(addprefix es2-,$(OBJECTS))

es1-%.o: $(dir $(GLESv1_CM_ASM))%.S
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<
es1-%.o: $(MESA)/glapi/%.c
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<

es2-%.o: $(dir $(GLESv2_ASM))%.S
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<
es2-%.o: $(MESA)/glapi/%.c
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<

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
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(API_SOURCES) \
		$(ES1_API_ASM)  $(ES2_API_ASM)

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
