TOP = ../../../..
MAPI = $(TOP)/src/mapi/mapi
GLAPI = ../gen
include $(TOP)/configs/current

OUTPUTS :=			\
	glapi_mapi_tmp.h	\
	main/dispatch.h		\
	main/remap_helper.h

COMMON = gl_and_es_API.xml gl_XML.py glX_XML.py license.py typeexpr.py
COMMON := $(addprefix $(GLAPI)/, $(COMMON))

ES1_APIXML := es1_API.xml
ES2_APIXML := es2_API.xml
ES1_OUTPUT_DIR := $(TOP)/src/mapi/es1api
ES2_OUTPUT_DIR := $(TOP)/src/mapi/es2api

ES1_DEPS = $(ES1_APIXML) base1_API.xml es1_EXT.xml es_EXT.xml \
	   es1_COMPAT.xml es_COMPAT.xml
ES2_DEPS = $(ES2_APIXML) base2_API.xml es2_EXT.xml es_EXT.xml \
	   es2_COMPAT.xml es_COMPAT.xml

ES1_OUTPUTS := $(addprefix $(ES1_OUTPUT_DIR)/, $(OUTPUTS))
ES2_OUTPUTS := $(addprefix $(ES2_OUTPUT_DIR)/, $(OUTPUTS))

SHARED_GLAPI_APIXML := $(GLAPI)/gl_and_es_API.xml
SHARED_GLAPI_OUTPUT_DIR := $(TOP)/src/mapi/shared-glapi
SHARED_GLAPI_DEPS := $(SHARED_GLAPI_APIXML)
SHARED_GLAPI_OUTPUTS = $(SHARED_GLAPI_OUTPUT_DIR)/glapi_mapi_tmp.h

all: es1 es2 shared-glapi

es1: $(ES1_OUTPUTS)
es2: $(ES2_OUTPUTS)
shared-glapi: $(SHARED_GLAPI_OUTPUTS)

$(ES1_OUTPUTS): APIXML := $(ES1_APIXML)
$(ES1_OUTPUTS): PRINTER := es1api
$(ES1_OUTPUTS): $(ES1_DEPS)

$(ES2_OUTPUTS): APIXML := $(ES2_APIXML)
$(ES2_OUTPUTS): PRINTER := es2api
$(ES2_OUTPUTS): $(ES2_DEPS)

$(SHARED_GLAPI_OUTPUTS): APIXML := $(SHARED_GLAPI_APIXML)
$(SHARED_GLAPI_OUTPUTS): PRINTER := shared-glapi
$(SHARED_GLAPI_OUTPUTS): $(SHARED_GLAPI_DEPS)

define gen-glapi
	@mkdir -p $(dir $@)
	$(PYTHON2) $(PYTHON_FLAGS) $< -f $(APIXML) $(1) > $@
endef

%/glapi_mapi_tmp.h: $(MAPI)/mapi_abi.py $(COMMON)
	@mkdir -p $(dir $@)
	$(PYTHON2) $(PYTHON_FLAGS) $< \
		--printer $(PRINTER) --mode lib $(GLAPI)/gl_and_es_API.xml > $@

%/main/dispatch.h: $(GLAPI)/gl_table.py $(COMMON)
	$(call gen-glapi,-c -m remap_table)

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

clean-es1:
	-rm -f $(ES1_OUTPUTS)
	-rm -rf $(ES1_OUTPUT_DIR)/main

clean-es2:
	-rm -f $(ES2_OUTPUTS)
	-rm -rf $(ES2_OUTPUT_DIR)/main

clean-shared-glapi:
	-rm -f $(SHARED_GLAPI_OUTPUTS)

clean: clean-es1 clean-es2 clean-shared-glapi
	-rm -f *~ *.pyc *.pyo
