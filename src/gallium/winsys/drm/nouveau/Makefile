# src/gallium/winsys/drm/nouveau/Makefile
TOP = ../../../../..
include $(TOP)/configs/current

SUBDIRS = drm $(GALLIUM_STATE_TRACKERS_DIRS)

default install clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) $@) || exit 1; \
		fi \
	done
