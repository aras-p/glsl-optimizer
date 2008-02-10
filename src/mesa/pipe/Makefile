TOP = ../../..
include $(TOP)/configs/current


ifeq ($(CONFIG_NAME), linux-cell)
CELL_DIR = cell
endif

ifeq ($(CONFIG_NAME), linux-llvm)
LLVM_DIR = llvm
endif

SUBDIRS = softpipe i915simple i965simple nv30 nv40 nv50 failover pipebuffer \
	  $(CELL_DIR) $(LLVM_DIR)


default: subdirs


subdirs:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE)) || exit 1 ; \
		fi \
	done


clean:
	rm -f `find . -name \*.[oa]`
