
TOP = ../../..
include $(TOP)/configs/current


ifeq ($(CONFIG_NAME), linux-cell)
CELL_DIR = cell
endif

SUBDIRS = softpipe i915simple nv40 nv50 failover $(CELL_DIR)


default: subdirs


subdirs:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE)) || exit 1 ; \
		fi \
	done


clean:
	rm -f `find . -name \*.[oa]`
