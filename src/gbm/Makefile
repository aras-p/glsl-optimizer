# src/gbm/Makefile

TOP = ../..
include $(TOP)/configs/current

SUBDIRS = backends main


default install clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) $@) || exit 1 ; \
		fi \
	done
