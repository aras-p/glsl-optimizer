# src/mesa/es/Makefile
#
TOP := ../../..
MESA := ..

include $(TOP)/configs/current
include sources.mak

ES1_LIBS := libes1gallium.a libes1api.a
ES2_LIBS := libes2gallium.a libes2api.a

# Default rule: create ES1 and ES2 libs
.PHONY: default es1 es2
default: depend es1 es2

es1: $(ES1_LIBS)
	@rm -f subdirs-stamp-tmp

es2: $(ES2_LIBS)
	@rm -f subdirs-stamp-tmp

# force the inclusion of es's mfeatures.h
ES1_CPPFLAGS := -include main/mfeatures_es1.h -D__GL_EXPORTS
ES2_CPPFLAGS := -include main/mfeatures_es2.h -D__GL_EXPORTS

ES1_OBJ_DIR := objs-es1
ES2_OBJ_DIR := objs-es2

# adjust output dirs
ES1_OBJECTS := $(addprefix $(ES1_OBJ_DIR)/, $(ES1_OBJECTS))
ES1_GALLIUM_OBJECTS := $(addprefix $(ES1_OBJ_DIR)/, $(ES1_GALLIUM_OBJECTS))
ES1_API_OBJECTS := $(addprefix $(ES1_OBJ_DIR)/, $(ES1_API_OBJECTS))

ES2_OBJECTS := $(addprefix $(ES2_OBJ_DIR)/, $(ES2_OBJECTS))
ES2_GALLIUM_OBJECTS := $(addprefix $(ES2_OBJ_DIR)/, $(ES2_GALLIUM_OBJECTS))
ES2_API_OBJECTS := $(addprefix $(ES2_OBJ_DIR)/, $(ES2_API_OBJECTS))

# compile either ES1 or ES2 sources
define es-compile
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(ES$(1)_CPPFLAGS) $(ES$(1)_INCLUDES) -o $@ $<
endef

$(ES1_OBJ_DIR)/%.o: %.c
	$(call es-compile,1)

$(ES1_OBJ_DIR)/%.o: %.S
	$(call es-compile,1)

$(ES1_OBJ_DIR)/%.o: $(MESA)/%.c
	$(call es-compile,1)

$(ES1_OBJ_DIR)/%.o: $(MESA)/%.S
	$(call es-compile,1)

$(ES2_OBJ_DIR)/%.o: %.c
	$(call es-compile,2)

$(ES2_OBJ_DIR)/%.o: %.S
	$(call es-compile,2)

$(ES2_OBJ_DIR)/%.o: $(MESA)/%.c
	$(call es-compile,2)

$(ES2_OBJ_DIR)/%.o: $(MESA)/%.S
	$(call es-compile,2)

libes1.a: $(ES1_OBJECTS) $(GLSL_LIBS)
	@$(MKLIB) -o es1 -static $(ES1_OBJECTS) $(GLSL_LIBS)

libes2.a: $(ES2_OBJECTS) $(GLSL_LIBS)
	@$(MKLIB) -o es2 -static $(ES1_OBJECTS) $(GLSL_LIBS)

libes1gallium.a: $(ES1_GALLIUM_OBJECTS) $(GLSL_LIBS)
	@$(MKLIB) -o es1gallium -static $(ES1_GALLIUM_OBJECTS) $(GLSL_LIBS)

libes2gallium.a: $(ES2_GALLIUM_OBJECTS) $(GLSL_LIBS)
	@$(MKLIB) -o es2gallium -static $(ES2_GALLIUM_OBJECTS) $(GLSL_LIBS)

libes1api.a: $(ES1_API_OBJECTS)
	@$(MKLIB) -o es1api -static $(ES1_API_OBJECTS)

libes2api.a: $(ES2_API_OBJECTS)
	@$(MKLIB) -o es2api -static $(ES2_API_OBJECTS)

GENERATED_SOURCES :=		\
	main/api_exec_es1.c	\
	main/api_exec_es2.c	\
	main/get_es1.c		\
	main/get_es2.c

main/api_exec_es1.c: main/APIspec.xml main/es_generator.py main/APIspecutil.py main/APIspec.py
	$(PYTHON2) $(PYTHON_FLAGS) main/es_generator.py -S main/APIspec.xml -V GLES1.1 > $@

main/api_exec_es2.c: main/APIspec.xml main/es_generator.py main/APIspecutil.py main/APIspec.py
	$(PYTHON2) $(PYTHON_FLAGS) main/es_generator.py -S main/APIspec.xml -V GLES2.0 > $@

main/get_es1.c: main/get_gen.py
	$(PYTHON2) $(PYTHON_FLAGS) $< 1 > $@

main/get_es2.c: main/get_gen.py
	$(PYTHON2) $(PYTHON_FLAGS) $< 2 > $@

.PHONY: clean
clean:
	-rm -f $(ES1_LIBS) $(ES2_LIBS)
	-rm -rf $(ES1_OBJ_DIR) $(ES2_OBJ_DIR)
	-rm -f $(GENERATED_SOURCES)
	-rm -f depend depend.bak
	-rm -f subdirs-stamp-tmp
	@$(MAKE) -C glapi clean

# nothing to install
install:

subdirs-stamp-tmp:
	@$(MAKE) -C $(MESA) asm_subdirs
	@$(MAKE) -C $(MESA) glsl_builtin
	@$(MAKE) -C glapi
	@touch subdirs-stamp-tmp

# sort to avoid duplicates
ALL_SOURCES := $(sort $(ES1_ALL_SOURCES) $(ES2_ALL_SOURCES))

# need to make sure the subdirs are processed first
$(ALL_SOURCES): | subdirs-stamp-tmp

depend: $(ALL_SOURCES)
	@echo "running $(MKDEP)"
	@touch depend
	@# MESA is "..", but luckily, directories are longer than 2 characters
	@$(MKDEP) -f- -p$(ES1_OBJ_DIR)/ $(DEFINES) $(ES1_CFLAGS) \
		$(ES1_INCLUDES) $(ES1_ALL_SOURCES) 2>/dev/null | \
		sed -e 's,^$(ES1_OBJ_DIR)/$(MESA)/,$(ES1_OBJ_DIR)/,' > depend
	@$(MKDEP) -f- -p$(ES2_OBJ_DIR)/ $(DEFINES) $(ES2_CFLAGS) \
		$(ES2_INCLUDES) $(ES2_ALL_SOURCES) 2>/dev/null | \
		sed -e 's,^$(ES2_OBJ_DIR)/$(MESA)/,$(ES2_OBJ_DIR)/,' >> depend

ifneq ($(MAKECMDGOALS),clean)
-include depend
endif
