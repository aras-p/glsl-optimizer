TOP = ../../..
include $(TOP)/configs/current


ifeq ($(CONFIG_NAME), linux-llvm)
LLVM_DIR = llvm
endif

SUBDIRS = pipebuffer $(LLVM_DIR)


default: subdirs


subdirs:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE)) || exit 1 ; \
		fi \
	done


clean:
	rm -f `find . -name \*.[oa]`
