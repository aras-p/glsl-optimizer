# src/glsl/Makefile

TOP = ../..

include $(TOP)/configs/current

SUBDIRS = pp cl apps

default install clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) $@) || exit 1; \
		fi \
	done

