# src/gbm/backends/Makefile

TOP = ../../..
include $(TOP)/configs/current

SUBDIRS = $(GBM_BACKEND_DIRS)


default install clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir ; $(MAKE) $@) || exit 1 ; \
		fi \
	done
