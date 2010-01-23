# src/gallium/state_trackers/es/Makefile

# Build the ES 1/2 state tracker libraries
# This consists of core Mesa ES, plus GL/gallium state tracker.

TOP = ../../../..
include $(TOP)/configs/current

GLES_1_VERSION_MAJOR = 1
GLES_1_VERSION_MINOR = 1
GLES_1_VERSION_PATCH = 0

GLES_2_VERSION_MAJOR = 2
GLES_2_VERSION_MINOR = 0
GLES_2_VERSION_PATCH = 0


# Maybe move these into configs/default:
GLES_1_LIB = GLESv1_CM
GLES_1_LIB_NAME = lib$(GLES_1_LIB).so
GLES_2_LIB = GLESv2
GLES_2_LIB_NAME = lib$(GLES_2_LIB).so


ES1_OBJECTS = st_es1.o
ES2_OBJECTS = st_es2.o


ES1_LIBS = \
	$(TOP)/src/mesa/es/libes1gallium.a \
	$(TOP)/src/mesa/es/libes1api.a

ES2_LIBS = \
	$(TOP)/src/mesa/es/libes2gallium.a \
	$(TOP)/src/mesa/es/libes2api.a

SYS_LIBS = -lm -pthread


INCLUDE_DIRS = \
	-I$(TOP)/src/gallium/include

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(DEFINES) $(CFLAGS) $< -o $@


# Default: make both GL ES 1.1 and GL ES 2.0 libraries
default: $(TOP)/$(LIB_DIR)/$(GLES_1_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLES_2_LIB_NAME)

# Make the shared libs
$(TOP)/$(LIB_DIR)/$(GLES_1_LIB_NAME): $(ES1_OBJECTS) $(ES1_LIBS) $(GALLIUM_AUXILIARIES)
	$(MKLIB) -o $(GLES_1_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(GLES_1_VERSION_MAJOR) \
		-minor $(GLES_1_VERSION_MINOR) \
		-patch $(GLES_1_VERSION_PATCH) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		$(ES1_OBJECTS) \
		-Wl,--whole-archive $(ES1_LIBS) -Wl,--no-whole-archive \
		$(GALLIUM_AUXILIARIES) $(SYS_LIBS)

$(TOP)/$(LIB_DIR)/$(GLES_2_LIB_NAME): $(ES2_OBJECTS) $(ES1_LIBS) $(GALLIUM_AUXILIARIES)
	$(MKLIB) -o $(GLES_2_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(GLES_2_VERSION_MAJOR) \
		-minor $(GLES_2_VERSION_MINOR) \
		-patch $(GLES_2_VERSION_PATCH) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		$(ES2_OBJECTS) \
		-Wl,--whole-archive $(ES2_LIBS) -Wl,--no-whole-archive \
		$(GALLIUM_AUXILIARIES) $(SYS_LIBS)

install: default
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/include/GLES
	$(INSTALL) -m 644 $(TOP)/include/GLES/*.h $(DESTDIR)$(INSTALL_DIR)/include/GLES
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/include/GLES2
	$(INSTALL) -m 644 $(TOP)/include/GLES2/*.h $(DESTDIR)$(INSTALL_DIR)/include/GLES2
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(MINSTALL) $(TOP)/$(LIB_DIR)/libGLESv1* $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(MINSTALL) $(TOP)/$(LIB_DIR)/libGLESv2* $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)

clean:
	-rm -f *.o *~
	-rm -f $(TOP)/$(LIB_DIR)/$(GLES_1_LIB_NAME)* $(TOP)/$(LIB_DIR)/$(GLES_2_LIB_NAME)*

depend:
