TOP = ../../../..
GLAPI = ../../glapi/gen
include $(TOP)/configs/current

OUTPUTS :=			\
	glapi/glapidispatch.h	\
	glapi/glapioffsets.h	\
	glapi/glapitable.h	\
	glapi/glapitemp.h	\
	glapi/glprocs.h		\
	sparc/glapi_sparc.S	\
	x86-64/glapi_x86-64.S	\
	x86/glapi_x86.S		\
	main/enums.c		\
	main/remap_helper.h

COMMON = gl_XML.py glX_XML.py license.py typeexpr.py
COMMON := $(addprefix $(GLAPI)/, $(COMMON))

ES1_APIXML := es1_API.xml
ES2_APIXML := es2_API.xml
ES1_OUTPUT_DIR := glapi-es1
ES2_OUTPUT_DIR := glapi-es2

ES1_DEPS = $(ES1_APIXML) base1_API.xml es1_EXT.xml es_EXT.xml \
	   es1_COMPAT.xml es_COMPAT.xml
ES2_DEPS = $(ES2_APIXML) base2_API.xml es2_EXT.xml es_EXT.xml \
	   es2_COMPAT.xml es_COMPAT.xml

ES1_OUTPUTS := $(addprefix $(ES1_OUTPUT_DIR)/, $(OUTPUTS))
ES2_OUTPUTS := $(addprefix $(ES2_OUTPUT_DIR)/, $(OUTPUTS))

all: $(ES1_OUTPUTS) $(ES2_OUTPUTS)

$(ES1_OUTPUTS): APIXML := $(ES1_APIXML)
$(ES2_OUTPUTS): APIXML := $(ES2_APIXML)
$(ES1_OUTPUTS): $(ES1_DEPS)
$(ES2_OUTPUTS): $(ES2_DEPS)

define gen-glapi
	@mkdir -p $(dir $@)
	$(PYTHON2) $(PYTHON_FLAGS) $< -f $(APIXML) $(1) > $@
endef

%/glapidispatch.h: $(GLAPI)/gl_table.py $(COMMON)
	$(call gen-glapi,-c -m remap_table)

%/glapioffsets.h: $(GLAPI)/gl_offsets.py $(COMMON)
	$(call gen-glapi,-c)

%/glapitable.h: $(GLAPI)/gl_table.py $(COMMON)
	$(call gen-glapi,-c)

%/glapitemp.h: $(GLAPI)/gl_apitemp.py $(COMMON)
	$(call gen-glapi,-c)

%/glprocs.h: $(GLAPI)/gl_procs.py $(COMMON)
	$(call gen-glapi,-c)

%/sparc/glapi_sparc.S: $(GLAPI)/gl_SPARC_asm.py $(COMMON)
	$(call gen-glapi)

%/x86-64/glapi_x86-64.S: $(GLAPI)/gl_x86-64_asm.py $(COMMON)
	$(call gen-glapi)

%/x86/glapi_x86.S: $(GLAPI)/gl_x86_asm.py $(COMMON)
	$(call gen-glapi)

%/main/enums.c: $(GLAPI)/gl_enums.py $(COMMON)
	$(call gen-glapi)

%/main/remap_helper.h: $(GLAPI)/remap_helper.py $(COMMON)
	$(call gen-glapi)

verify_xml:
	@if [ ! -f gl.h ]; then \
		echo "Please copy gl.h and gl2.h to this directory"; \
		exit 1; \
	fi
	@echo "Verifying that es1_API.xml covers OpenGL ES 1.1..."
	@$(PYTHON2) $(PYTHON_FLAGS) gl_parse_header.py gl.h > tmp.xml
	@$(PYTHON2) $(PYTHON_FLAGS) gl_compare.py difference tmp.xml es1_API.xml
	@echo "Verifying that es2_API.xml covers OpenGL ES 2.0..."
	@$(PYTHON2) $(PYTHON_FLAGS) gl_parse_header.py gl2.h > tmp.xml
	@$(PYTHON2) $(PYTHON_FLAGS) gl_compare.py difference tmp.xml es2_API.xml
	@rm -f tmp.xml

clean:
	-rm -rf $(ES1_OUTPUT_DIR) $(ES2_OUTPUT_DIR)
	-rm -f *~ *.pyc *.pyo
