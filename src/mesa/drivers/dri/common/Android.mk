#
# Mesa 3-D graphics library
#
# Copyright (C) 2011 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#

#
# Build libmesa_dri_common
#

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/Makefile.sources

LOCAL_MODULE := libmesa_dri_common
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

intermediates := $(call local-intermediates-dir)

LOCAL_C_INCLUDES := \
    $(intermediates) \
    $(MESA_DRI_C_INCLUDES)

LOCAL_SRC_FILES := $(mesa_dri_common_SOURCES)

LOCAL_GENERATED_SOURCES := \
    $(intermediates)/xmlpool/options.h

#
# Generate options.h from gettext translations.
#

MESA_DRI_OPTIONS_LANGS := de es nl fr sv
POT := $(intermediates)/xmlpool.pot

$(POT): $(LOCAL_PATH)/xmlpool/t_options.h
	@mkdir -p $(dir $@)
	xgettext -L C --from-code utf-8 -o $@ $<

$(intermediates)/xmlpool/%.po: $(LOCAL_PATH)/xmlpool/%.po $(POT)
	lang=$(basename $(notdir $@)); \
	mkdir -p $(dir $@); \
	if [ -f $< ]; then \
		msgmerge -o $@ $^; \
	else \
		msginit -i $(POT) \
			-o $@ \
			--locale=$$lang \
			--no-translator; \
		sed -i -e 's/charset=.*\\n/charset=UTF-8\\n/' $@; \
	fi

$(intermediates)/xmlpool/%/LC_MESSAGES/options.mo: $(intermediates)/xmlpool/%.po
	mkdir -p $(dir $@)
	msgfmt -o $@ $<

$(intermediates)/xmlpool/options.h: PRIVATE_SCRIPT := $(LOCAL_PATH)/xmlpool/gen_xmlpool.py
$(intermediates)/xmlpool/options.h: PRIVATE_LOCALEDIR := $(intermediates)/xmlpool
$(intermediates)/xmlpool/options.h: PRIVATE_TEMPLATE_HEADER := $(LOCAL_PATH)/xmlpool/t_options.h
$(intermediates)/xmlpool/options.h: PRIVATE_MO_FILES := $(MESA_DRI_OPTIONS_LANGS:%=$(intermediates)/xmlpool/%/LC_MESSAGES/options.mo)
$(intermediates)/xmlpool/options.h: $(PRIVATE_SCRIPT) $(PRIVATE_TEMPLATE_HEADER) $(PRIVATE_MO_FILES)
	mkdir -p $(dir $@)
	mkdir -p $(PRIVATE_LOCALEDIR)
	$(MESA_PYTHON2) $(PRIVATE_SCRIPT) $(PRIVATE_TEMPLATE_HEADER) \
		$(PRIVATE_LOCALEDIR) $(MESA_DRI_OPTIONS_LANGS) > $@

include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)
