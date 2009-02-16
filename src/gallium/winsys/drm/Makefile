# src/gallium/winsys/drm/Makefile

TOP = ../../../..

include $(TOP)/configs/current

SUBDIRS = $(GALLIUM_WINSYS_DRM_DIRS)

default: $(TOP)/$(LIB_DIR)/gallium subdirs


$(TOP)/$(LIB_DIR)/gallium:
	-mkdir -p $(TOP)/$(LIB_DIR)/gallium


subdirs:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE)) || exit 1 ; \
		fi \
	done


install:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) install) || exit 1 ; \
		fi \
	done


clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) clean) ; \
		fi \
	done
	-rm -f common/*.o
