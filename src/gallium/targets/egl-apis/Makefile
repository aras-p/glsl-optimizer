# src/gallium/targets/egl-apis

TOP = ../../../..
include $(TOP)/configs/current

OUTPUT_PREFIX := api_
OUTPUT_PATH := $(TOP)/$(LIB_DIR)/egl

OUTPUTS := $(addsuffix .so, $(EGL_CLIENT_APIS))
OUTPUTS := $(addprefix $(OUTPUT_PATH)/$(OUTPUT_PREFIX), $(OUTPUTS))

# include dirs
GL_INCLUDES := -I$(TOP)/src/mesa -I$(TOP)/src/gallium/include
GLESv1_CM_INCLUDES := $(GL_INCLUDES)
GLESv2_INCLUDES := $(GL_INCLUDES)
OpenVG_INCLUDES := -I$(TOP)/src/gallium/state_trackers/vega -I$(TOP)/src/gallium/include

# system libs
GL_SYS := -lpthread -lm -L$(TOP)/$(LIB_DIR) -l$(GL_LIB)
GLESv1_CM_SYS := -lpthread -lm -L$(TOP)/$(LIB_DIR) -l$(GLESv1_CM_LIB)
GLESv2_SYS := -lpthread -lm -L$(TOP)/$(LIB_DIR) -l$(GLESv2_LIB)
OpenVG_SYS := -lm -L$(TOP)/$(LIB_DIR) -l$(VG_LIB)

# project libs
GL_LIBS := $(TOP)/src/mesa/libmesagallium.a
GLESv1_CM_LIBS := $(TOP)/src/mesa/es/libes1gallium.a
GLESv2_LIBS := $(TOP)/src/mesa/es/libes2gallium.a
OpenVG_LIBS := $(TOP)/src/gallium/state_trackers/vega/libvega.a

# objects
GL_OBJECTS := api_GL.o
GLESv1_CM_OBJECTS := api_GLESv1_CM.o
GLESv2_OBJECTS := api_GLESv2.o
OpenVG_OBJECTS := api_OpenVG.o

default: $(OUTPUTS)

api_%.o: api_%.c
	$(CC) -c -o $@ $< $($*_INCLUDES) $(DEFINES) $(CFLAGS)

define mklib
$(MKLIB) -o $(notdir $@) -noprefix -linker '$(CC)' -ldflags '$(LDFLAGS)' \
	-install $(OUTPUT_PATH) $(MKLIB_OPTIONS) \
	$($(1)_OBJECTS) $($(1)_LIBS) $(GALLIUM_AUXILIARIES) $($(1)_SYS)
endef

$(OUTPUT_PATH)/$(OUTPUT_PREFIX)$(GL_LIB).so: $(GL_OBJECTS) $(GL_LIBS)
	$(call mklib,GL)

$(OUTPUT_PATH)/$(OUTPUT_PREFIX)$(GLESv1_CM_LIB).so: $(GLESv1_CM_OBJECTS) $(GLESv1_CM_LIBS)
	$(call mklib,GLESv1_CM)

$(OUTPUT_PATH)/$(OUTPUT_PREFIX)$(GLESv2_LIB).so: $(GLESv2_OBJECTS) $(GLESv2_LIBS)
	$(call mklib,GLESv2)

$(OUTPUT_PATH)/$(OUTPUT_PREFIX)$(VG_LIB).so: $(OpenVG_OBJECTS) $(OpenVG_LIBS)
	$(call mklib,OpenVG)

install: $(OUTPUTS)
	$(INSTALL) -d $(DESTDIR)$(EGL_DRIVER_INSTALL_DIR)
	for out in $(OUTPUTS); do \
		$(MINSTALL) -m 755 "$$out" $(DESTDIR)$(EGL_DRIVER_INSTALL_DIR); \
	done

clean:
	-rm -f $(OUTPUTS)
	-rm -f *.o
