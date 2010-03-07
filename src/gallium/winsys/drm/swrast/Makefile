# src/gallium/winsys/drm/swrast/Makefile
TOP = ../../../../..
include $(TOP)/configs/current

SUBDIRS = core $(GALLIUM_STATE_TRACKERS_DIRS)

default install clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) $@) || exit 1; \
		fi \
	done
