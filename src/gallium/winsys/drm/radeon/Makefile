# src/gallium/winsys/drm/radeon/Makefile
TOP = ../../../../..
include $(TOP)/configs/current

SUBDIRS = core dri2 egl xorg

default install clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) $@) || exit 1; \
		fi \
	done
