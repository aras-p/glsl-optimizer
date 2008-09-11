TOP = ../../..
include $(TOP)/configs/current


SUBDIRS = $(GALLIUM_WINSYS_DIRS)


default: subdirs


subdirs:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE)) || exit 1 ; \
		fi \
	done


clean:
	rm -f `find . -name \*.[oa]`


# Dummy install target
install:
